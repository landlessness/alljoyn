#include "nan.h"

#include "util.h"
#include "SignalHandlerImpl.h"
#include <Message.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/AllJoynStd.h>

#include <algorithm>

SignalHandlerImpl::SignalHandlerImpl(NanCallback* sig){
  loop = uv_default_loop();
  signalCallback.callback = sig;
  uv_async_init(loop, &signal_async, signal_callback);
}

SignalHandlerImpl::~SignalHandlerImpl(){
}

template<typename... Args>
void SignalHandlerImpl::signal_callback(uv_async_t *handle, Args...) {
  CallbackHolder* holder = (CallbackHolder*) handle->data;

  std::queue<std::unique_ptr<ajn::Message>> messages;
  uv_mutex_lock(&holder->lock);
  messages = std::move(holder->messages);
  uv_mutex_unlock(&holder->lock);

  while(!messages.empty()) {
    std::unique_ptr<ajn::Message> message = std::move(messages.front());
    v8::Local<v8::Object> msg = v8::Object::New();
    size_t msgIndex = 0;
    const ajn::MsgArg* arg = (*message)->GetArg(msgIndex);
    while(arg != NULL){
      msgArgToObject(arg, msgIndex, msg);
      msgIndex++;
      arg = (*message)->GetArg(msgIndex);
    }

    v8::Local<v8::Object> sender = v8::Object::New();
    sender->Set(NanNew<v8::String>("sender"), NanNew<v8::String>((*message)->GetSender()));
    sender->Set(NanNew<v8::String>("session_id"), NanNew<v8::Integer>((*message)->GetSessionId()));
    sender->Set(NanNew<v8::String>("timestamp"), NanNew<v8::Integer>((*message)->GetTimeStamp()));
    sender->Set(NanNew<v8::String>("member_name"), NanNew<v8::String>((*message)->GetMemberName()));
    sender->Set(NanNew<v8::String>("object_path"), NanNew<v8::String>((*message)->GetObjectPath()));
    sender->Set(NanNew<v8::String>("signature"), NanNew<v8::String>((*message)->GetSignature()));

    v8::Handle<v8::Value> argv[] = {
      msg,
      sender
    };
    holder->callback->Call(2, argv);
    messages.pop();
  }
}

void SignalHandlerImpl::Signal(const ajn::InterfaceDescription::Member *member, const char *srcPath, ajn::Message &message){
  signal_async.data = (void*) &signalCallback;

  uv_mutex_lock(&signalCallback.lock);
  signalCallback.messages.emplace(new ajn::Message(message));
  uv_mutex_unlock(&signalCallback.lock);

  uv_async_send(&signal_async);
}
