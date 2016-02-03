// globals
global.assert = require('assert');

global.aboutListenerWasCalled = false;
global.clientBusAttachment = null;

global.sessionlessData = {};
global.sessionfulData = {};

// setup
var alljoyn = require('../');
var xml = require('xml2json');

var util = require('util');


var ALL_GOOD = 0;
var SESSION_PORT = 900;


var MESSAGE_INVALID     = 0; ///< an invalid message type
var MESSAGE_METHOD_CALL = 1; ///< a method call message type
var MESSAGE_METHOD_RET  = 2; ///< a method return message type
var MESSAGE_ERROR       = 3; ///< an error message type
var MESSAGE_SIGNAL      = 4; ///< a signal message type


var heartbeatSignalHandler = null;
var heartbeatSignalHandlerAlreadyCalled = false;
var heartbeatSignalName = 'Heartbeat';

var clientApplicationName = 'AboutPlusServiceTest';

var setupClientBusAttachment = function(clientApplicationName) {
  // sanity check test suite
  assert.equal(true, true);
  // sanity check AllJoyn bus
  assert.equal(typeof alljoyn.BusAttachment, 'function');

  // for this suite to work, the AllJoyn AboutPlusService sample must be running
  // how to run it? after running npm install, run from the command line:
  // ./build/Release/sample-about-plus-service
  clientBusAttachment = alljoyn.BusAttachment(clientApplicationName, true);
  assert.equal(typeof clientBusAttachment, 'object');

  // start the bus attachment
  assert.equal(clientBusAttachment.start(), ALL_GOOD);

  // connect to bus
  assert.equal(clientBusAttachment.connect(), ALL_GOOD);
  
  return clientBusAttachment;
}

var sessionPortListenerCallback = alljoyn.SessionPortListener(
  function(port, joiner){
    if (port != SESSION_PORT) {
      return false;
    } else {
      return true;
    }
  },
  function(port, sessionId, joiner){
  }
);

var argumentsForSignature = function(signature) {
  [1];
}

before(function(done){

  // setup the service Bus Attachment with the Application Name
  // setup the client Bus Attachment with the Application Name
  clientBusAttachment = setupClientBusAttachment(clientApplicationName);
  var clientBusUniqueName = clientBusAttachment.getUniqueName();

  //TODO: do some assertion testing on the two bus attachments.
  assert.equal(typeof(clientBusUniqueName), 'string');
  assert.equal(clientBusUniqueName.length, 11);

  // create an Announced callback that will get called
  // after the AboutListener is registered and the
  // WhoImplements function is called
  var announcedCallback = function(busName, version, port, objectDescription, aboutData){
    aboutListenerWasCalled = true;
    sessionlessData.busName = busName;
    sessionlessData.version = version;
    sessionlessData.port = port;
    sessionlessData.objectDescription = objectDescription;
    sessionlessData.aboutData = aboutData;

    // once the Announced callback has fired let's go ahead and 
    // join the session and get more About info
    var sessionId = 0;

    sessionId = clientBusAttachment.joinSession(busName, port, sessionId);
    // if the returned sessionId a string then it's an error message
    // number is good
    assert.equal(typeof(sessionId),'number');

    // let's get the About proxy
    var aboutProxy = alljoyn.AboutProxy(clientBusAttachment, busName, sessionId);
    assert.equal(typeof(aboutProxy), 'object');

    sessionfulData.sessionId = aboutProxy.getSessionId();
    assert.equal(sessionfulData.sessionId, sessionId);
    sessionfulData.uniqueName = aboutProxy.getUniqueName();
    assert.equal(sessionfulData.uniqueName, busName);

    // now that we have an About Proxy we can grab the Object Description,
    // About Data and Version.
    sessionfulData.version = aboutProxy.getVersion();
    sessionfulData.objectDescription = aboutProxy.getObjectDescription();
    sessionfulData.aboutData = aboutProxy.getAboutData('en');
    sessionfulData.interfaceNamesForPath = {};    

    var paths = Object.keys(sessionlessData.objectDescription);
    for (i = 0; i < paths.length; i++) {
      var proxyBusObject = alljoyn.ProxyBusObject(clientBusAttachment, busName, paths[i], sessionId);
      var interfaceNames = proxyBusObject.getInterfaceNames();
      sessionfulData.interfaceNamesForPath[paths[i]] = {descriptionForInterfaceName: {}};
      
      assert.equal(interfaceNames.length, 4);
      assert.equal(interfaceNames[0], 'com.se.bus.discovery');
      assert.equal(interfaceNames[1], 'org.allseen.Introspectable');
      assert.equal(interfaceNames[2], 'org.freedesktop.DBus.Introspectable');
      assert.equal(interfaceNames[3], 'org.freedesktop.DBus.Peer');
      var numberOfMembersPerInterface = [37, 2, 1, 2];
      var numberOfSignalsPerInterface = [30, 0, 0, 0];
      var numberOfMethodsPerInterface = [19, 2, 0, 1];
      for (j = 0; j < interfaceNames.length; j++) {
        var methodCallCount = 0;
        var serviceInterfaceDescription = alljoyn.InterfaceDescription();
        proxyBusObject.getInterface(interfaceNames[j], serviceInterfaceDescription);
        assert.notEqual(serviceInterfaceDescription, null);
        var description = xml.toJson(serviceInterfaceDescription.introspect(), {object: true});
        sessionfulData.interfaceNamesForPath[paths[i]].descriptionForInterfaceName[interfaceNames[j]] = {descriptionFromXML: description, alljoynInterfaceDescription: serviceInterfaceDescription};

        // test signals
        var signals = ('signal' in description.interface) ? description.interface.signal : [];
        var signalOutArgs = [];
        for (s = 0; s < signals.length; s++) {
          var signal = signals[s];
          assert.equal(typeof(signal.name), 'string');
          assert(signal.name.length > 0);
          if ('arg' in signal) {
            var args = ('length' in signal.arg) ? signal.arg : [signal.arg];
            for (a = 0; a < args.length; a++) {
              var arg = args[a];
              assert(/^out$/.test(arg.direction));
              assert.equal(typeof(arg.type), 'string');
              signalOutArgs.push({signature: arg.type, value: arg.value});
            }
          }
        }
        assert.equal(signalOutArgs.length,numberOfSignalsPerInterface[j]);

        // test methods
        var methods = ('method' in description.interface) ? description.interface.method : [];
        for (m = 0; m < methods.length; m++) {
          var method = methods[m];
          assert.equal(typeof(method.name), 'string');
          assert(method.name.length > 0);
          validateInArg = function(arg) {
            assert.equal(typeof(arg.name), 'string');
            assert(arg.name.length > 0);
            assert.equal(typeof(arg.type), 'string');
            assert(arg.type.length > 0);
            assert(/^in$|^out$/.test(arg.direction));
          };
          testValueForType = function(argType) {
            switch (argType) {
            case 's':
              return 'Foolicious'
            case 'u':
              return 1
            default:
            }
          }
          validateOutArg =function(actual, expected, methodName) {
            if ('GetDescriptionLanguages' == method.name) {return;}
            //TODO: change MethodCall to return Array then do assertions here.
            var keys = Object.keys(actual);
            for (k = 0; k < keys.length; k++) {
              assert.equal(keys[k], expected[k].name);
            }
            assert.equal(Object.keys(actual).length,outArgs.length);
          }
          if ('arg' in method) {
            var inArgs = [];
            var outArgs = [];
            var args = ('length' in method.arg) ? method.arg : [method.arg];
            for (a = 0; a < args.length; a++) {
              var arg = args[a];
              if (/^in$/.test(arg.direction)) {
                validateInArg(arg);
                inArgs.push({signature: arg.type, value: testValueForType(arg.type)});
              } else if (/^out$/.test(arg.direction)) {
                outArgs.push({signature: arg.type, name: arg.name});
              }
            }
            methodCallCount++; 
            var methodResponse = proxyBusObject.methodCall(clientBusAttachment, interfaceNames[j], method.name, inArgs, outArgs);
            assert.equal(typeof(methodResponse), 'object');
            var keys = Object.keys(methodResponse);
            validateOutArg(methodResponse, outArgs, method.name);
          }
        }
        assert.equal(methodCallCount, numberOfMethodsPerInterface[j]);
        var members = serviceInterfaceDescription.getMembers();
        assert.equal(members.length,numberOfMembersPerInterface[j]);
        for (k = 0; k < members.length; k++) {
          var member = members[k];
          assert.notEqual(member, null);
          assert.equal(typeof(member.memberType), 'number');
          assert.equal(typeof(member.name), 'string');
          assert.equal(typeof(member.signature), 'string');
          assert.equal(typeof(member.returnSignature), 'string');
          assert.equal(typeof(member.argNames), 'string');
          assert.equal(typeof(member.accessPerms), 'string');
          assert.equal(typeof(member.description), 'string');
          assert.equal(typeof(member.isSessionlessSignal), 'boolean');
          assert.equal(Object.keys(member).length,8);
          if (member.memberType == MESSAGE_METHOD_CALL) {
          }
        }
      }
    }

    heartbeatSignalHandler = function(msg, sender){
      if (heartbeatSignalHandlerAlreadyCalled) {
      } else {
        assert(/^bump$|^ba$/.test(msg['0']));
        assert.equal(typeof(sender.sender), 'string');
        assert.equal(sender.sender.length, 11);
        assert.equal(typeof(sender.session_id), 'number');
        assert.equal(typeof(sender.timestamp), 'number');
        assert.equal(sender.member_name, heartbeatSignalName);
        assert.equal(sender.object_path, BUS_OBJECT_PATH);
        assert.equal(sender.signature, 's');
      
        heartbeatSignalHandlerAlreadyCalled = true;
        done();
      }
    };

    // register heartbeat signal handler
    var busObject = alljoyn.BusObject(BUS_OBJECT_PATH);
    var busInterface = alljoyn.InterfaceDescription();
    proxyBusObject.getInterface(interfaceNames[0], busInterface);
    assert.equal(clientBusAttachment.registerSignalHandler(busObject, heartbeatSignalHandler, busInterface, heartbeatSignalName), ALL_GOOD);
  }

  // create a new About Listener
  var aboutListener = alljoyn.AboutListener(announcedCallback);

  //register the About Listener with the Announced Callback
  assert.equal(clientBusAttachment.registerAboutListener(aboutListener), undefined);
  
  // call WhoImplements on the service interface name 
  // to trigger the Announced Callback
  assert.equal(clientBusAttachment.whoImplements([SERVICE_INTERFACE_NAME]), ALL_GOOD);
  
});

