/******************************************************************************
* Copyright AllSeen Alliance. All rights reserved.
*
*    Permission to use, copy, modify, and/or distribute this software for any
*    purpose with or without fee is hereby granted, provided that the above
*    copyright notice and this permission notice appear in all copies.
*
*    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
*    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
*    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
*    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
*    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
*    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
*    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
******************************************************************************/

#include <alljoyn/AboutObj.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusListener.h>
#include <alljoyn/BusObject.h>
// #include <alljoyn/Init.h>

#include <signal.h>
#include <stdio.h>
#include <string>

using namespace std;
using namespace qcc;
using namespace ajn;

static volatile sig_atomic_t s_interrupt = false;

static void CDECL_CALL SigIntHandler(int sig) {
  QCC_UNUSED(sig);
  s_interrupt = true;
}

static SessionPort ASSIGNED_SESSION_PORT = 900;
static const char* INTERFACE_NAME = "com.se.bus.discovery";
static const InterfaceDescription::Member * circuitStatusSignal;
static const InterfaceDescription::Member * heartbeatSignal;
static SessionId s_sessionId;

class MySessionPortListener : public SessionPortListener {
  bool AcceptSessionJoiner(ajn::SessionPort sessionPort, const char* joiner, const ajn::SessionOpts& opts)
  {
    QCC_UNUSED(joiner);
    QCC_UNUSED(opts);

    if (sessionPort != ASSIGNED_SESSION_PORT) {
      printf("Rejecting join attempt on unexpected session port %d\n", sessionPort);
      return false;
    }
    return true;
  }
  void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner)
  {
    QCC_UNUSED(sessionPort);
    QCC_UNUSED(joiner);

    printf("Session Joined SessionId = %u\n", id);
    s_sessionId = id;
  }
};

class MyBusObject : public BusObject {
public:
  MyBusObject(BusAttachment& bus, const char* path)
  : BusObject(path) {
    QStatus status;
    const InterfaceDescription* iface;
    CreateInterface(bus, &iface);
    assert(iface != NULL);

    // Here the value ANNOUNCED tells AllJoyn that this interface
    // should be announced
    status = AddInterface(*iface, ANNOUNCED);
    if (status != ER_OK) {
      printf("Failed to add %s interface to the BusObject\n", INTERFACE_NAME);
    }

    /* Register the method handlers with the object */
    const MethodEntry methodEntries[] = {
      //{ iface->GetMember("Echo"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::Echo) },
      //{ iface->GetMember("cat"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::Cat) },
  		{ iface->GetMember("ReadDeviceStatus"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::ReadDeviceStatus) },
  		{ iface->GetMember("ReadBreakerType"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::ReadBreakerType) },
  		{ iface->GetMember("ReadLastTrip"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::ReadLastTrip) },
  		{ iface->GetMember("ReadInstantaneousDraw"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::ReadInstantaneousDraw) },
  		{ iface->GetMember("ReadEnergyDelivered"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::ReadEnergyDelivered) },
  		{ iface->GetMember("ReadCircuitLoad"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::ReadCircuitLoad) },
  		//{ iface->GetMember("WriteCircuitLoad"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::WriteCircuitLoad) },
  		{ iface->GetMember("TurnCircuitOn"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::TurnCircuitOn) },
  		{ iface->GetMember("TurnCircuitOff"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::TurnCircuitOff) },
  		{ iface->GetMember("ReadCircuitStatus"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::ReadCircuitStatus) },
  		{ iface->GetMember("ReadBranchName"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::ReadBranchName) },
  		{ iface->GetMember("WriteBranchName"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::WriteBranchName) },
  		//{ iface->GetMember("DownloadPanelConfig"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::DownloadPanelConfig) },
  		{ iface->GetMember("ReadCloudStatus"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::ReadCloudStatus) },
  		{ iface->GetMember("ReadPoleNumber"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::ReadPoleNumber) },
  		{ iface->GetMember("ReadBreakerCommStatus"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::ReadBreakerCommStatus) },
  		//{ iface->GetMember("ReadHarvesterTime"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::ReadHarvesterTime) },
  		{ iface->GetMember("ReadHarvesterInstalled"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::ReadHarvesterInstalled) },
  		{ iface->GetMember("ReadHarvesterConnType"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::ReadHarvesterConnType) },
  		//{ iface->GetMember("ReadHarvesterPowerState"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::ReadHarvesterPowerState) },
  		{ iface->GetMember("ReadRemoteControlState"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::ReadRemoteControlState) },
  		{ iface->GetMember("ReadUSBStatus"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::ReadUSBStatus) },
  		{ iface->GetMember("ChangeCredentialsRequest"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::ChangeCredentialsRequest) },
    };
    AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));

    circuitStatusSignal = iface->GetMember("CircuitStatusChanged");
    heartbeatSignal = iface->GetMember("Heartbeat");
  }

  /** Create the interface, report the result to stdout, and return the result status. */
  QStatus CreateInterface(BusAttachment& bus, const InterfaceDescription** pIntf)
  {
  	/* Add org.alljoyn.Bus.method_sample interface */
  	InterfaceDescription* testIntf = NULL;
  	QStatus status = bus.CreateInterface(INTERFACE_NAME, testIntf, AJ_IFC_SECURITY_OFF);

  	if (status == ER_OK) {
  		printf("Interface created.\n");
  		testIntf->AddMethod("ReadDeviceStatus", "u", "s", "Branch,DeviceStatus", 0);
  		testIntf->AddMemberAnnotation("ReadDeviceStatus", "IsStatusMethod", "true");
  		testIntf->AddMethod("ReadBreakerType", "u", "s", "Branch,BreakerType", 0);
  		testIntf->AddMemberAnnotation("ReadBreakerType", "IsStatusMethod", "true");
  		testIntf->AddMethod("ReadLastTrip", "u", "ss", "Branch,LastTrip,LastTripTime", 0);
  		testIntf->AddMemberAnnotation("ReadLastTrip", "IsStatusMethod", "true");
  		testIntf->AddMethod("ReadInstantaneousDraw", "u", "ss", "Branch,Amperage,Wattage", 0);
  		testIntf->AddMemberAnnotation("ReadInstantaneousDraw", "IsStatusMethod", "true");
  		testIntf->AddMethod("ReadEnergyDelivered", "u", "s", "Branch,EnergyDelivered", 0);
  		testIntf->AddMemberAnnotation("ReadEnergyDelivered", "IsStatusMethod", "true");
  		testIntf->AddMethod("ReadCircuitLoad", "u", "s", "Branch,CircuitLoad", 0);
  		testIntf->AddMemberAnnotation("ReadCircuitLoad", "IsStatusMethod", "true");
  		testIntf->AddMethod("TurnCircuitOn", "u", "u", "Branch,CircuitStatus", 0);
  		testIntf->AddMemberAnnotation("TurnCircuitOn", "IsStatusMethod", "false");
  		testIntf->AddMemberAnnotation("TurnCircuitOn", "ChangesPhysicalWorld", "true");

      // INTENTIONALLY NOT ADDING ANNOTATIONS to TurnCircuitOff 
      // in order to test the undefined case
  		testIntf->AddMethod("TurnCircuitOff", "u", "u", "Branch,CircuitStatus", 0);
      // ...
      // INTENTIONALLY NOT ADDING ANNOTATIONS to TurnCircuitOff

  		testIntf->AddMethod("ReadCircuitStatus", "u", "s", "Branch,CircuitStatus", 0);
  		testIntf->AddMemberAnnotation("ReadCircuitStatus", "IsStatusMethod", "true");
  		testIntf->AddMethod("ReadBranchName", "u", "s", "Branch,BranchName", 0);
  		testIntf->AddMemberAnnotation("ReadBranchName", "IsStatusMethod", "true");
  		testIntf->AddMethod("WriteBranchName", "us", "s", "Branch,BranchName_In,BranchName_Out", 0);
  		//testIntf->AddMethod("DownloadPanelConfig", NULL, "u", "Status", 0);
  		testIntf->AddMethod("ReadCloudStatus", NULL, "u", "CloudStatus", 0);
  		testIntf->AddMemberAnnotation("ReadCloudStatus", "IsStatusMethod", "true");
  		testIntf->AddMethod("ReadPoleNumber", "u", "s", "Branch,PoleNumber", 0);
  		testIntf->AddMemberAnnotation("ReadPoleNumber", "IsStatusMethod", "true");
  		testIntf->AddMethod("ReadBreakerCommStatus", "u", "s", "Branch,BreakerCommStatus", 0);
  		testIntf->AddMemberAnnotation("ReadBreakerCommStatus", "IsStatusMethod", "true");
  		//testIntf->AddMethod("ReadHarvesterTime", NULL, "x", "HarvesterTime", 0);
  		testIntf->AddMethod("ReadHarvesterInstalled", NULL, "s", "HarvesterInstalled", 0);
  		testIntf->AddMemberAnnotation("ReadHarvesterInstalled", "IsStatusMethod", "true");
  		testIntf->AddMethod("ReadHarvesterConnType", NULL, "s", "HarvesterConnType", 0);
  		testIntf->AddMemberAnnotation("ReadHarvesterConnType", "IsStatusMethod", "true");
  		//testIntf->AddMethod("ReadHarvesterPowerState", NULL, "s", "HarvesterPowerState", 0);
  		testIntf->AddMethod("ReadRemoteControlState", NULL, "s", "RemoteControlState", 0);
  		testIntf->AddMemberAnnotation("ReadRemoteControlState", "IsStatusMethod", "true");
  		testIntf->AddMethod("ReadUSBStatus", NULL, "s", "USBStatus", 0);
  		testIntf->AddMemberAnnotation("ReadUSBStatus", "IsStatusMethod", "true");
  		testIntf->AddMethod("ChangeCredentialsRequest", "ss", "u", "NewUserName,NewPassword,RetStatus", 0);

  		testIntf->AddSignal("DeviceStatusChanged", "us", "Branch,DeviceStatus", 0);
  		testIntf->AddSignal("Heartbeat", "s", "Pulse", 0);
  		testIntf->AddSignal("BreakerTypeChanged", "us", "Branch,BreakerType", 0);
  		testIntf->AddSignal("LastTripChanged", "uss", "Branch,LastTrip,LastTripTime", 0);
  		testIntf->AddSignal("InstantaneousDrawChanged", "uss", "Branch,Amperage,Wattage", 0);
  		testIntf->AddSignal("EnergyDeliveredChanged", "us", "Branch,EnergyDelivered", 0);
  		testIntf->AddSignal("CircuitLoadChanged", "us", "Branch,CircuitLoad", 0);
  		testIntf->AddSignal("CircuitStatusChanged", "us", "Branch,CircuitStatus", 0);
  		testIntf->AddSignal("BranchNameChanged", "us", "Branch,BranchName", 0);
  		testIntf->AddSignal("CloudStatusChanged", "u", "CloudStatus", 0);
  		testIntf->AddSignal("PoleNumberChanged", "us", "Branch,PoleNumber", 0);
  		testIntf->AddSignal("BreakerCommStatusChanged", "us", "Branch,BreakerCommStatus", 0);
  		testIntf->AddSignal("HarvesterTimeChanged", "x", "HarvesterTime", 0);
  		testIntf->AddSignal("HarvesterInstalledChanged", "s", "HarvesterInstalled", 0);
  		testIntf->AddSignal("HarvesterConnTypeChanged", "s", "HarvesterConnType", 0);
  		//testIntf->AddSignal("HarvesterPowerStateChanged", "s", "HarvesterPowerState", 0);
  		testIntf->AddSignal("RemoteControlStateChanged", "s", "RemoteControlState", 0);
  		testIntf->AddSignal("USBStatusChanged", "s", "USBStatus", 0);
  		testIntf->AddSignal("BroadcastAlert", "s", "AlertMsg", 0);

  		testIntf->Activate();

  		*pIntf = testIntf;

  		// Test print interface data in XML format
  		printf("%s\n", testIntf->Introspect().c_str());
  	}
  	else {
  		printf("Failed to create interface '%s'.\n", INTERFACE_NAME);
  	}

  	return status;
  }

  void ReadDeviceStatus(const InterfaceDescription::Member* member, Message& msg) {
    QCC_UNUSED(member);

    uint32_t branchNumber = msg->GetArg(0)->v_uint32;
    string outStr = "Active";

    printf("ReadDeviceStatus: %d\n", branchNumber);

    MsgArg returnArg("s", outStr.c_str());
    const MsgArg* replyArg(&returnArg);
    QStatus status = MethodReply(msg, replyArg, 1);
    if (status != ER_OK) {
      printf("Failed to created MethodReply.\n");
    }
  }

  void ReadBreakerType(const InterfaceDescription::Member* member, Message& msg) {
    QCC_UNUSED(member);

    uint32_t branchNumber = msg->GetArg(0)->v_uint32;
    string outStr = "GFI";

    printf("ReadBreakerType: %d\n", branchNumber);

    MsgArg returnArg("s", outStr.c_str());
    const MsgArg* replyArg(&returnArg);
    QStatus status = MethodReply(msg, replyArg, 1);
    if (status != ER_OK) {
      printf("Failed to created MethodReply.\n");
    }
  }

  void ReadLastTrip(const InterfaceDescription::Member* member, Message& msg) {
    QCC_UNUSED(member);

    uint32_t branchNumber = msg->GetArg(0)->v_uint32;
    string outStr1 = "Test";
    string outStr2 = "2016-01-01 12:00:00";

    printf("ReadLastTrip: %d\n", branchNumber);

    MsgArg returnArg[2];
    returnArg[0].Set("s", outStr1.c_str());
    returnArg[1].Set("s", outStr2.c_str());
    const MsgArg* replyArg(returnArg);
    QStatus status = MethodReply(msg, replyArg, 2);
    if (status != ER_OK) {
      printf("Failed to created MethodReply.\n");
    }
  }

  void ReadInstantaneousDraw(const InterfaceDescription::Member* member, Message& msg) {
    QCC_UNUSED(member);

    uint32_t branchNumber = msg->GetArg(0)->v_uint32;
    string outStr1 = "2.234";
    string outStr2 = "110.5";

    printf("ReadInstantaneousDraw: %d\n", branchNumber);

    MsgArg returnArg[2];
    returnArg[0].Set("s", outStr1.c_str());
    returnArg[1].Set("s", outStr2.c_str());
    const MsgArg* replyArg(returnArg);
    QStatus status = MethodReply(msg, replyArg, 2);
    if (status != ER_OK) {
      printf("Failed to created MethodReply.\n");
    }
  }

  void ReadEnergyDelivered(const InterfaceDescription::Member* member, Message& msg) {
    QCC_UNUSED(member);

    uint32_t branchNumber = msg->GetArg(0)->v_uint32;
    string outStr = "123.456";

    printf("ReadEnergyDelivered: %d\n", branchNumber);

    MsgArg returnArg("s", outStr.c_str());
    const MsgArg* replyArg(&returnArg);
    QStatus status = MethodReply(msg, replyArg, 1);
    if (status != ER_OK) {
      printf("Failed to created MethodReply.\n");
    }
  }

  void ReadCircuitLoad(const InterfaceDescription::Member* member, Message& msg) {
    QCC_UNUSED(member);

    uint32_t branchNumber = msg->GetArg(0)->v_uint32;
    string outStr = "Heater";

    printf("ReadCircuitLoad: %d\n", branchNumber);

    MsgArg returnArg("s", outStr.c_str());
    const MsgArg* replyArg(&returnArg);
    QStatus status = MethodReply(msg, replyArg, 1);
    if (status != ER_OK) {
      printf("Failed to created MethodReply.\n");
    }
  }
  
  void SendHeartbeatSignal(const char* onomatopoeia) {
    MsgArg signalArg[1];
    signalArg[0].Set("s", onomatopoeia);
    uint8_t flags = 0;
    printf("%s \n", onomatopoeia);
    Signal(NULL, s_sessionId, *heartbeatSignal, signalArg, 1, 0, flags);
  }  

  void TurnCircuitOn(const InterfaceDescription::Member* member, Message& msg) {
    QCC_UNUSED(member);

    uint32_t branchNumber = msg->GetArg(0)->v_uint32;
    uint32_t outInt = 1;

    printf("TurnCircuitOn: %d\n", branchNumber);

    MsgArg returnArg("u", outInt);
    const MsgArg* replyArg(&returnArg);
    QStatus status = MethodReply(msg, replyArg, 1);
    if (status != ER_OK) {
      printf("Failed to created MethodReply.\n");
    }

    MsgArg signalArg[2];
    signalArg[0].Set("u", branchNumber);
    signalArg[1].Set("s", "On");
    uint8_t flags = 0;
    Signal(NULL, s_sessionId, *circuitStatusSignal, signalArg, 2, 0, flags);
  }

  void TurnCircuitOff(const InterfaceDescription::Member* member, Message& msg) {
    QCC_UNUSED(member);

    uint32_t branchNumber = msg->GetArg(0)->v_uint32;
    uint32_t outInt = 1;

    printf("TurnCircuitOff: %d\n", branchNumber);

    MsgArg returnArg("u", outInt);
    const MsgArg* replyArg(&returnArg);
    QStatus status = MethodReply(msg, replyArg, 1);
    if (status != ER_OK) {
      printf("Failed to created MethodReply.\n");
    }

    MsgArg signalArg[2];
    signalArg[0].Set("u", branchNumber);
    signalArg[1].Set("s", "Off");
    uint8_t flags = 0;
    Signal(NULL, s_sessionId, *circuitStatusSignal, signalArg, 2, 0, flags);
  }

  void ReadCircuitStatus(const InterfaceDescription::Member* member, Message& msg) {
    QCC_UNUSED(member);

    uint32_t branchNumber = msg->GetArg(0)->v_uint32;
    string outStr = "On";

    printf("ReadCircuitStatus: %d\n", branchNumber);

    MsgArg returnArg("s", outStr.c_str());
    const MsgArg* replyArg(&returnArg);
    QStatus status = MethodReply(msg, replyArg, 1);
    if (status != ER_OK) {
      printf("Failed to created MethodReply.\n");
    }
  }

  void ReadBranchName(const InterfaceDescription::Member* member, Message& msg) {
    QCC_UNUSED(member);

    uint32_t branchNumber = msg->GetArg(0)->v_uint32;
    string outStr = "Main #1";

    printf("ReadBranchName: %d\n", branchNumber);

    MsgArg returnArg("s", outStr.c_str());
    const MsgArg* replyArg(&returnArg);
    QStatus status = MethodReply(msg, replyArg, 1);
    if (status != ER_OK) {
      printf("Failed to created MethodReply.\n");
    }
  }

  void WriteBranchName(const InterfaceDescription::Member* member, Message& msg) {
    QCC_UNUSED(member);

    uint32_t branchNumber = msg->GetArg(0)->v_uint32;
    String branchName = msg->GetArg(1)->v_string.str;
    string outStr = "WRITE_SUCCESS";

    printf("WriteBranchName: %d:%s\n", branchNumber, branchName.c_str());

    MsgArg returnArg("s", outStr.c_str());
    const MsgArg* replyArg(&returnArg);
    QStatus status = MethodReply(msg, replyArg, 1);
    if (status != ER_OK) {
      printf("Failed to created MethodReply.\n");
    }
  }

  void ReadCloudStatus(const InterfaceDescription::Member* member, Message& msg) {
    QCC_UNUSED(member);

    uint32_t outInt = 0;

    printf("ReadCloudStatus\n");

    MsgArg returnArg("u", outInt);
    const MsgArg* replyArg(&returnArg);
    QStatus status = MethodReply(msg, replyArg, 1);
    if (status != ER_OK) {
      printf("Failed to created MethodReply.\n");
    }
  }

  void ReadPoleNumber(const InterfaceDescription::Member* member, Message& msg) {
    QCC_UNUSED(member);

    uint32_t branchNumber = msg->GetArg(0)->v_uint32;
    string outStr = "2P";

    printf("ReadPoleNumber: %d\n", branchNumber);

    MsgArg returnArg("s", outStr.c_str());
    const MsgArg* replyArg(&returnArg);
    QStatus status = MethodReply(msg, replyArg, 1);
    if (status != ER_OK) {
      printf("Failed to created MethodReply.\n");
    }
  }

  void ReadBreakerCommStatus(const InterfaceDescription::Member* member, Message& msg) {
    QCC_UNUSED(member);

    uint32_t branchNumber = msg->GetArg(0)->v_uint32;
    string outStr = "Connected";

    printf("ReadBreakerCommStatus: %d\n", branchNumber);

    MsgArg returnArg("s", outStr.c_str());
    const MsgArg* replyArg(&returnArg);
    QStatus status = MethodReply(msg, replyArg, 1);
    if (status != ER_OK) {
      printf("Failed to created MethodReply.\n");
    }
  }

  void ReadHarvesterInstalled(const InterfaceDescription::Member* member, Message& msg) {
    QCC_UNUSED(member);

    string outStr = "Installed";

    printf("ReadHarvesterInstalled\n");

    MsgArg returnArg("s", outStr.c_str());
    const MsgArg* replyArg(&returnArg);
    QStatus status = MethodReply(msg, replyArg, 1);
    if (status != ER_OK) {
      printf("Failed to created MethodReply.\n");
    }
  }

  void ReadHarvesterConnType(const InterfaceDescription::Member* member, Message& msg) {
    QCC_UNUSED(member);

    string outStr = "Ethernet";

    printf("ReadHarvesterConnType\n");

    MsgArg returnArg("s", outStr.c_str());
    const MsgArg* replyArg(&returnArg);
    QStatus status = MethodReply(msg, replyArg, 1);
    if (status != ER_OK) {
      printf("Failed to created MethodReply.\n");
    }
  }

  void ReadRemoteControlState(const InterfaceDescription::Member* member, Message& msg) {
    QCC_UNUSED(member);

    string outStr = "Active";

    printf("ReadRemoteControlState\n");

    MsgArg returnArg("s", outStr.c_str());
    const MsgArg* replyArg(&returnArg);
    QStatus status = MethodReply(msg, replyArg, 1);
    if (status != ER_OK) {
      printf("Failed to created MethodReply.\n");
    }
  }

  void ReadUSBStatus(const InterfaceDescription::Member* member, Message& msg) {
    QCC_UNUSED(member);

    string outStr = "Inactive";

    printf("ReadUSBStatus\n");

    MsgArg returnArg("s", outStr.c_str());
    const MsgArg* replyArg(&returnArg);
    QStatus status = MethodReply(msg, replyArg, 1);
    if (status != ER_OK) {
      printf("Failed to created MethodReply.\n");
    }
  }

  void ChangeCredentialsRequest(const InterfaceDescription::Member* member, Message& msg) {
    QCC_UNUSED(member);

    String NewUsername = msg->GetArg(0)->v_string.str;
  	String NewPassword = msg->GetArg(1)->v_string.str;
    uint32_t RetStatus = 0;

    printf("ChangeCredentialsRequest: %s:%s\n", NewUsername.c_str(), NewPassword.c_str());

    MsgArg returnArg("u", RetStatus);
    const MsgArg* replyArg(&returnArg);
    QStatus status = MethodReply(msg, replyArg, 1);
    if (status != ER_OK) {
      printf("Failed to created MethodReply.\n");
    }
  }
};

/** Main entry point */
int CDECL_CALL main(int argc, char** argv)
{
  QCC_UNUSED(argc);
  QCC_UNUSED(argv);

  // if (AllJoynInit() != ER_OK) {
  //   return 1;
  // }
  // #ifdef ROUTER
  // if (AllJoynRouterInit() != ER_OK) {
  //   AllJoynShutdown();
  //   return 1;
  // }
  // #endif
  //
  /* Install SIGINT handler so Ctrl + C deallocates memory properly */
  signal(SIGINT, SigIntHandler);

  QStatus status;

  BusAttachment* bus = new BusAttachment("Discovery", true);
  status = bus->Start();
  if (ER_OK == status) {
    printf("BusAttachment started.\n");
  } else {
    printf("FAILED to start BusAttachment (%s)\n", QCC_StatusText(status));
    exit(1);
  }

  status = bus->Connect();
  if (ER_OK == status) {
    printf("BusAttachment connect succeeded. BusName %s\n", bus->GetUniqueName().c_str());
  } else {
    printf("FAILED to connect to router node (%s)\n", QCC_StatusText(status));
    exit(1);
  }

  SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
  SessionPort sessionPort = ASSIGNED_SESSION_PORT;
  MySessionPortListener sessionPortListener;
  status = bus->BindSessionPort(sessionPort, opts, sessionPortListener);
  if (ER_OK != status) {
    printf("Failed to BindSessionPort (%s)", QCC_StatusText(status));
    exit(1);
  }

  // Setup the about data
  // The default language is specified in the constructor. If the default language
  // is not specified any Field that should be localized will return an error
  AboutData aboutData("en");
  //AppId is a 128bit uuid
  uint8_t appId[] = { 0x01, 0xB3, 0xBA, 0x14,
    0x1E, 0x82, 0x11, 0xE4,
    0x86, 0x51, 0xD1, 0x56,
    0x1D, 0x5D, 0x46, 0xB0 };
    status = aboutData.SetAppId(appId, 16);
    status = aboutData.SetDeviceName("DiscoveryAJService");
    //DeviceId is a string encoded 128bit UUID
    status = aboutData.SetDeviceId("93c06771-c725-48c2-b1ff-6a2a59d445b8");
    status = aboutData.SetAppName("Discovery");
    status = aboutData.SetManufacturer("Schneider Electric");
    status = aboutData.SetModelNumber("0001");
    status = aboutData.SetDescription("The AllJoyn service for Discovery interfacing");
    status = aboutData.SetDateOfManufacture("2016-01-07");
    status = aboutData.SetSoftwareVersion("1.0.0");
    status = aboutData.SetHardwareVersion("1.0.0");
    status = aboutData.SetSupportUrl("http://www.schneider-electric.org");

    // Check to see if the aboutData is valid before sending the About Announcement
    if (!aboutData.IsValid()) {
      printf("failed to setup about data.\n");
    }

    MyBusObject* busObject = new MyBusObject(*bus, "/example/path");

    status = bus->RegisterBusObject(*busObject);
    if (ER_OK != status) {
      printf("Failed to register BusObject (%s)", QCC_StatusText(status));
      exit(1);
    }

    // Announce about signal
    AboutObj* aboutObj = new AboutObj(*bus);
    // Note the ObjectDescription that is part of the Announce signal is found
    // automatically by introspecting the BusObjects registered with the bus
    // attachment.
    status = aboutObj->Announce(sessionPort, aboutData);
    if (ER_OK == status) {
      printf("AboutObj Announce Succeeded.\n");
    } else {
      printf("AboutObj Announce failed (%s)\n", QCC_StatusText(status));
      exit(1);
    }

    /* Perform the service asynchronously until the user signals for an exit. */
    if (ER_OK == status) {
      while (s_interrupt == false) {
        busObject->SendHeartbeatSignal("ba");
        #ifdef _WIN32
        Sleep(10);
        #else
        usleep(1 * 1000);
        #endif
        busObject->SendHeartbeatSignal("bump");
        #ifdef _WIN32
        Sleep(10);
        #else
        usleep(1 * 1000);
        #endif
      }
    }

    aboutObj->Unannounce();
    delete aboutObj;
    delete busObject;
    delete bus;
    #ifdef ROUTER
    AllJoynRouterShutdown();
    #endif
    // AllJoynShutdown();
    return 0;
  }
