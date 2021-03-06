/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/
#include <gtest/gtest.h>
#include <time.h>
#include <alljoyn_c/ApplicationStateListener.h>
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/DBusStdDefines.h>
#include <alljoyn_c/InterfaceDescription.h>
#include <alljoyn_c/ProxyBusObject.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>
#if defined(QCC_OS_GROUP_WINDOWS)
#include <qcc/windows/NamedPipeWrapper.h>
#endif
#include "ajTestCommon.h"
#include "InMemoryKeyStore.h"

/*
 * The unit test uses a busy wait loop. The busy wait loops were chosen
 * over thread sleeps because of the ease of understanding the busy wait loops.
 * Also busy wait loops do not require any platform specific threading code.
 */
#define WAIT_MSECS 5
#define STATE_CHANGE_TIMEOUT_MS 2000

#define SECURITY_AGENT_BUS_NAME "SecurityAgentBus"
#define MANAGED_APP_BUS_NAME "SampleManagedApp"

static AJ_PCSTR s_allowAllManifestTemplate =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</method>"
    "<property>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</property>"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</signal>"
    "</interface>"
    "</node>"
    "</manifest>";

static AJ_PCSTR s_busAttachmentTestName = "BusAttachmentTest";
static AJ_PCSTR s_otherBusAttachmentTestName = "BusAttachment OtherBus";

using namespace ajn;
using namespace qcc;
using namespace std;

class BusAttachmentSecurity20Test : public testing::Test {
  public:

    BusAttachmentSecurity20Test() :
        m_securityAgent(nullptr),
        m_managedApp(nullptr)
    {
        memset(&m_callbacks, 0, sizeof(m_callbacks));
        m_callbacks.state = stateCallback;
    };

    virtual void SetUp()
    {
        BasicBusSetup(&m_securityAgent, SECURITY_AGENT_BUS_NAME, &m_securityAgentKeyStoreListener);
        BasicBusSetup(&m_managedApp, MANAGED_APP_BUS_NAME, &m_managedAppKeyStoreListener);
        SetupAgent();
    }

    virtual void TearDown()
    {
        BasicBusTearDown(m_securityAgent);
        BasicBusTearDown(m_managedApp);
    }

  protected:

    alljoyn_busattachment m_securityAgent;

    void CreateApplicationStateListener(alljoyn_applicationstatelistener* listener, bool* listenerCalled = nullptr)
    {
        *listener = alljoyn_applicationstatelistener_create(&m_callbacks, listenerCalled);
        ASSERT_NE(nullptr, listener);
    }

    void ChangeApplicationState()
    {
        ASSERT_EQ(ER_OK, alljoyn_busattachment_enablepeersecurity(m_managedApp,
                                                                  "ALLJOYN_ECDHE_NULL",
                                                                  nullptr,
                                                                  nullptr,
                                                                  QCC_TRUE));
        SetManifestTemplate(m_managedApp);
    }

    bool WaitForTrueOrTimeout(bool* isTrue, int timeoutMs)
    {
        time_t startTime = time(nullptr);
        int timeDiffMs = difftime(time(nullptr), startTime) * 1000;
        while (!(*isTrue)
               && (timeDiffMs < timeoutMs)) {
            qcc::Sleep(WAIT_MSECS);
            timeDiffMs = difftime(time(nullptr), startTime) * 1000;
        }

        return *isTrue;
    }

  private:

    alljoyn_busattachment m_managedApp;
    alljoyn_applicationstatelistener_callbacks m_callbacks;
    InMemoryKeyStoreListener m_securityAgentKeyStoreListener;
    InMemoryKeyStoreListener m_managedAppKeyStoreListener;

    static void AJ_CALL stateCallback(AJ_PCSTR busName,
                                      AJ_PCSTR publicKey,
                                      alljoyn_applicationstate applicationState,
                                      void* listenerCalled)
    {
        QCC_UNUSED(busName);
        QCC_UNUSED(publicKey);
        QCC_UNUSED(applicationState);

        if (listenerCalled) {
            *((bool*)listenerCalled) = true;
        }
    }

    void SetupAgent()
    {
        ASSERT_EQ(ER_OK, alljoyn_busattachment_enablepeersecurity(m_securityAgent,
                                                                  "ALLJOYN_ECDHE_NULL",
                                                                  nullptr,
                                                                  nullptr,
                                                                  QCC_TRUE));
    }

    void BasicBusSetup(alljoyn_busattachment* bus, AJ_PCSTR busName, InMemoryKeyStoreListener* keyStoreListener)
    {
        *bus = alljoyn_busattachment_create(busName, QCC_FALSE);
        ASSERT_EQ(ER_OK, alljoyn_busattachment_start(*bus));
        ASSERT_EQ(ER_OK, alljoyn_busattachment_connect(*bus, getConnectArg().c_str()));
        ASSERT_EQ(ER_OK, alljoyn_busattachment_registerkeystorelistener(*bus, (alljoyn_keystorelistener)keyStoreListener));
    }

    void BasicBusTearDown(alljoyn_busattachment bus)
    {
        ASSERT_EQ(ER_OK, alljoyn_busattachment_stop(bus));
        ASSERT_EQ(ER_OK, alljoyn_busattachment_join(bus));
        alljoyn_busattachment_destroy(bus);
    }

    void SetManifestTemplate(alljoyn_busattachment bus)
    {
        alljoyn_permissionconfigurator configurator = alljoyn_busattachment_getpermissionconfigurator(bus);
        ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setmanifesttemplatefromxml(configurator, s_allowAllManifestTemplate));
    }
};

TEST_F(BusAttachmentSecurity20Test, shouldReturnNonNullPermissionConfigurator)
{
    EXPECT_NE(nullptr, alljoyn_busattachment_getpermissionconfigurator(m_securityAgent));
}

TEST_F(BusAttachmentSecurity20Test, shouldReturnErrorWhenRegisteringWithNullListener)
{
    EXPECT_EQ(ER_INVALID_ADDRESS, alljoyn_busattachment_registerapplicationstatelistener(m_securityAgent, nullptr));
}

TEST_F(BusAttachmentSecurity20Test, shouldReturnErrorWhenUnregisteringWithNullListener)
{
    EXPECT_EQ(ER_INVALID_ADDRESS, alljoyn_busattachment_unregisterapplicationstatelistener(m_securityAgent, nullptr));
}

TEST_F(BusAttachmentSecurity20Test, shouldReturnErrorWhenUnregisteringUnknownListener)
{
    alljoyn_applicationstatelistener listener = nullptr;
    CreateApplicationStateListener(&listener);

    EXPECT_EQ(ER_APPLICATION_STATE_LISTENER_NO_SUCH_LISTENER, alljoyn_busattachment_unregisterapplicationstatelistener(m_securityAgent, listener));

    alljoyn_applicationstatelistener_destroy(listener);
}

TEST_F(BusAttachmentSecurity20Test, shouldRegisterSuccessfullyForNewListener)
{
    alljoyn_applicationstatelistener listener = nullptr;
    CreateApplicationStateListener(&listener);

    EXPECT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(m_securityAgent, listener));

    alljoyn_applicationstatelistener_destroy(listener);
}

TEST_F(BusAttachmentSecurity20Test, shouldUnregisterSuccessfullyForSameListener)
{
    alljoyn_applicationstatelistener listener = nullptr;
    CreateApplicationStateListener(&listener);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(m_securityAgent, listener));
    EXPECT_EQ(ER_OK, alljoyn_busattachment_unregisterapplicationstatelistener(m_securityAgent, listener));

    alljoyn_applicationstatelistener_destroy(listener);
}

TEST_F(BusAttachmentSecurity20Test, shouldReturnErrorWhenRegisteringSameListenerTwice)
{
    alljoyn_applicationstatelistener listener = nullptr;
    CreateApplicationStateListener(&listener);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(m_securityAgent, listener));
    EXPECT_EQ(ER_APPLICATION_STATE_LISTENER_ALREADY_EXISTS, alljoyn_busattachment_registerapplicationstatelistener(m_securityAgent, listener));

    alljoyn_applicationstatelistener_destroy(listener);
}

TEST_F(BusAttachmentSecurity20Test, shouldReturnErrorWhenUnregisteringSameListenerTwice)
{
    alljoyn_applicationstatelistener listener = nullptr;
    CreateApplicationStateListener(&listener);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(m_securityAgent, listener));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_unregisterapplicationstatelistener(m_securityAgent, listener));
    EXPECT_EQ(ER_APPLICATION_STATE_LISTENER_NO_SUCH_LISTENER, alljoyn_busattachment_unregisterapplicationstatelistener(m_securityAgent, listener));

    alljoyn_applicationstatelistener_destroy(listener);
}

TEST_F(BusAttachmentSecurity20Test, shouldRegisterSameListenerSuccessfullyAfterUnregister)
{
    alljoyn_applicationstatelistener listener = nullptr;
    CreateApplicationStateListener(&listener);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(m_securityAgent, listener));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_unregisterapplicationstatelistener(m_securityAgent, listener));

    EXPECT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(m_securityAgent, listener));

    alljoyn_applicationstatelistener_destroy(listener);
}

TEST_F(BusAttachmentSecurity20Test, shouldCallStateListenerAfterRegister)
{
    bool listenerCalled = false;
    alljoyn_applicationstatelistener listener = nullptr;
    CreateApplicationStateListener(&listener, &listenerCalled);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(m_securityAgent, listener));
    ChangeApplicationState();

    EXPECT_TRUE(WaitForTrueOrTimeout(&listenerCalled, STATE_CHANGE_TIMEOUT_MS));

    alljoyn_applicationstatelistener_destroy(listener);
}

TEST_F(BusAttachmentSecurity20Test, shouldNotCallStateListenerAfterUnregister)
{
    bool listenerCalled = false;
    alljoyn_applicationstatelistener listener = nullptr;
    CreateApplicationStateListener(&listener, &listenerCalled);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(m_securityAgent, listener));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_unregisterapplicationstatelistener(m_securityAgent, listener));
    ChangeApplicationState();

    EXPECT_FALSE(WaitForTrueOrTimeout(&listenerCalled, STATE_CHANGE_TIMEOUT_MS));

    alljoyn_applicationstatelistener_destroy(listener);
}

TEST_F(BusAttachmentSecurity20Test, shouldCallAllStateListeners)
{
    bool firstListenerCalled = false;
    bool secondListenerCalled = false;
    alljoyn_applicationstatelistener firstListener = nullptr;
    alljoyn_applicationstatelistener secondListener = nullptr;
    CreateApplicationStateListener(&firstListener, &firstListenerCalled);
    CreateApplicationStateListener(&secondListener, &secondListenerCalled);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(m_securityAgent, firstListener));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(m_securityAgent, secondListener));
    ChangeApplicationState();

    EXPECT_TRUE(WaitForTrueOrTimeout(&firstListenerCalled, STATE_CHANGE_TIMEOUT_MS));
    EXPECT_TRUE(WaitForTrueOrTimeout(&secondListenerCalled, STATE_CHANGE_TIMEOUT_MS));

    alljoyn_applicationstatelistener_destroy(firstListener);
    alljoyn_applicationstatelistener_destroy(secondListener);
}

TEST_F(BusAttachmentSecurity20Test, shouldCallOnlyOneStateListenerWhenOtherUnregistered)
{
    bool firstListenerCalled = false;
    bool secondListenerCalled = false;
    alljoyn_applicationstatelistener firstListener = nullptr;
    alljoyn_applicationstatelistener secondListener = nullptr;
    CreateApplicationStateListener(&firstListener, &firstListenerCalled);
    CreateApplicationStateListener(&secondListener, &secondListenerCalled);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(m_securityAgent, firstListener));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerapplicationstatelistener(m_securityAgent, secondListener));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_unregisterapplicationstatelistener(m_securityAgent, firstListener));
    ChangeApplicationState();

    EXPECT_FALSE(WaitForTrueOrTimeout(&firstListenerCalled, STATE_CHANGE_TIMEOUT_MS));
    EXPECT_TRUE(WaitForTrueOrTimeout(&secondListenerCalled, STATE_CHANGE_TIMEOUT_MS));

    alljoyn_applicationstatelistener_destroy(firstListener);
    alljoyn_applicationstatelistener_destroy(secondListener);
}

TEST(BusAttachmentTest, createinterface) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_FALSE);
    EXPECT_EQ(ER_OK, alljoyn_busattachment_deletedefaultkeystore(s_busAttachmentTestName));
    ASSERT_TRUE(bus != NULL);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.BusAttachment", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, deleteinterface) {
    QStatus status = ER_OK;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_FALSE);
    EXPECT_EQ(ER_OK, alljoyn_busattachment_deletedefaultkeystore(s_busAttachmentTestName));
    ASSERT_TRUE(bus != NULL);
    alljoyn_interfacedescription testIntf = NULL;
    status = alljoyn_busattachment_createinterface(bus, "org.alljoyn.test.BusAttachment", &testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_deleteinterface(bus, testIntf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, start_stop_join) {
    QStatus status = ER_FAIL;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_FALSE);
    EXPECT_EQ(ER_OK, alljoyn_busattachment_deletedefaultkeystore(s_busAttachmentTestName));
    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, isstarted_isstopping) {
    QStatus status = ER_FAIL;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_FALSE);
    EXPECT_EQ(ER_OK, alljoyn_busattachment_deletedefaultkeystore(s_busAttachmentTestName));
    EXPECT_EQ(QCC_FALSE, alljoyn_busattachment_isstarted(bus));
    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(QCC_TRUE, alljoyn_busattachment_isstarted(bus));
    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /*
     * Assumption made that the isstopping function will be called before all of
     * the BusAttachement threads have completed so it will return QCC_TRUE it is
     * possible, but unlikely, that this could return QCC_FALSE.
     */

    EXPECT_EQ(QCC_TRUE, alljoyn_busattachment_isstopping(bus));
    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(QCC_FALSE, alljoyn_busattachment_isstarted(bus));
    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, getconcurrency) {
    alljoyn_busattachment bus = NULL;
    unsigned int concurrency = (unsigned int)-1;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, alljoyn_busattachment_deletedefaultkeystore(s_busAttachmentTestName));

    concurrency = alljoyn_busattachment_getconcurrency(bus);
    //The default value for getconcurrency is 4
    EXPECT_EQ(4u, concurrency) << "  Expected a concurrency of 4 got " << concurrency;

    alljoyn_busattachment_destroy(bus);

    bus = NULL;
    concurrency = (unsigned int)-1;

    bus = alljoyn_busattachment_create_concurrency(s_busAttachmentTestName, QCC_TRUE, 8);
    EXPECT_EQ(ER_OK, alljoyn_busattachment_deletedefaultkeystore(s_busAttachmentTestName));

    concurrency = alljoyn_busattachment_getconcurrency(bus);
    //The default value for getconcurrency is 8
    EXPECT_EQ(8u, concurrency) << "  Expected a concurrency of 8 got " << concurrency;

    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, isconnected)
{
    QStatus status;
    alljoyn_busattachment bus;
    size_t i;

    QCC_BOOL allow_remote[2] = { QCC_FALSE, QCC_TRUE };

    for (i = 0; i < ArraySize(allow_remote); i++) {
        status = ER_FAIL;
        bus = NULL;

        bus = alljoyn_busattachment_create(s_busAttachmentTestName, allow_remote[i]);
        EXPECT_EQ(ER_OK, alljoyn_busattachment_deletedefaultkeystore(s_busAttachmentTestName));

        status = alljoyn_busattachment_start(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_FALSE(alljoyn_busattachment_isconnected(bus));

        status = alljoyn_busattachment_connect(bus, getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        if (ER_OK == status) {
            EXPECT_TRUE(alljoyn_busattachment_isconnected(bus));
        }

        status = alljoyn_busattachment_disconnect(bus, getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        if (ER_OK == status) {
            EXPECT_FALSE(alljoyn_busattachment_isconnected(bus));
        }

        status = alljoyn_busattachment_stop(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_busattachment_join(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        alljoyn_busattachment_destroy(bus);
    }
}

TEST(BusAttachmentTest, disconnect)
{
    QStatus status;
    alljoyn_busattachment bus;
    size_t i;

    QCC_BOOL allow_remote[2] = { QCC_FALSE, QCC_TRUE };

    for (i = 0; i < ArraySize(allow_remote); i++) {
        status = ER_FAIL;
        bus = NULL;

        bus = alljoyn_busattachment_create(s_busAttachmentTestName, allow_remote[i]);
        EXPECT_EQ(ER_OK, alljoyn_busattachment_deletedefaultkeystore(s_busAttachmentTestName));

        status = alljoyn_busattachment_disconnect(bus, NULL);
        EXPECT_EQ(ER_BUS_BUS_NOT_STARTED, status);

        status = alljoyn_busattachment_start(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_FALSE(alljoyn_busattachment_isconnected(bus));

        status = alljoyn_busattachment_disconnect(bus, NULL);
        EXPECT_EQ(ER_BUS_NOT_CONNECTED, status);

        status = alljoyn_busattachment_connect(bus, getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        if (ER_OK == status) {
            EXPECT_TRUE(alljoyn_busattachment_isconnected(bus));
        }

        status = alljoyn_busattachment_disconnect(bus, getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        if (ER_OK == status) {
            EXPECT_FALSE(alljoyn_busattachment_isconnected(bus));
        }

        status = alljoyn_busattachment_stop(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_busattachment_join(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        alljoyn_busattachment_destroy(bus);
    }
}

TEST(BusAttachmentTest, connect_null)
{
    QStatus status = ER_OK;

    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, alljoyn_busattachment_deletedefaultkeystore(s_busAttachmentTestName));

    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, NULL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(alljoyn_busattachment_isconnected(bus));

    AJ_PCSTR connectspec = alljoyn_busattachment_getconnectspec(bus);

    /**
     * Note: the default connect spec here must match the one in alljoyn_core BusAttachment.
     */
    AJ_PCSTR preferredConnectSpec;

#if defined(QCC_OS_GROUP_WINDOWS)
    if (NamedPipeWrapper::AreApisAvailable()) {
        preferredConnectSpec = "npipe:";
    } else {
        preferredConnectSpec = "tcp:addr=127.0.0.1,port=9955";
    }
#else
    preferredConnectSpec = "unix:abstract=alljoyn";
#endif

    /*
     * The BusAttachment has joined either a separate daemon (preferredConnectSpec) or
     * it is using the null transport (in bundled router).  If the null transport is used, the
     * connect spec will be 'null:' otherwise it will match the preferred default connect spec.
     */
    EXPECT_TRUE(strcmp(preferredConnectSpec, connectspec) == 0 ||
                strcmp("null:", connectspec) == 0);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, getconnectspec)
{
    QStatus status = ER_OK;

    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, alljoyn_busattachment_deletedefaultkeystore(s_busAttachmentTestName));

    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    AJ_PCSTR connectspec = alljoyn_busattachment_getconnectspec(bus);

    /*
     * The BusAttachment has joined either a separate daemon or it is using
     * the in process name service.  If the internal name service is used
     * the connect spec will be 'null:' otherwise it will match the ConnectArg.
     */
    EXPECT_TRUE(strcmp(getConnectArg().c_str(), connectspec) == 0 ||
                strcmp("null:", connectspec) == 0);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, getdbusobject) {
    QStatus status = ER_OK;

    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, alljoyn_busattachment_deletedefaultkeystore(s_busAttachmentTestName));

    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_proxybusobject dBusProxyObject = alljoyn_busattachment_getdbusproxyobj(bus);

    alljoyn_msgarg msgArgs = alljoyn_msgarg_array_create(2);
    status = alljoyn_msgarg_set(alljoyn_msgarg_array_element(msgArgs, 0), "s", "org.alljoyn.test.BusAttachment");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_msgarg_set(alljoyn_msgarg_array_element(msgArgs, 1), "u", 7u);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_message replyMsg = alljoyn_message_create(bus);

    status = alljoyn_proxybusobject_methodcall(dBusProxyObject, "org.freedesktop.DBus", "RequestName", msgArgs, 2, replyMsg, 25000, 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    unsigned int requestNameReply;
    alljoyn_msgarg reply = alljoyn_message_getarg(replyMsg, 0);
    alljoyn_msgarg_get(reply, "u", &requestNameReply);

    EXPECT_EQ(DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER, requestNameReply);

    alljoyn_msgarg_destroy(msgArgs);
    alljoyn_message_destroy(replyMsg);

    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, ping_self) {
    QStatus status;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, alljoyn_busattachment_deletedefaultkeystore(s_busAttachmentTestName));

    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_ping(bus, alljoyn_busattachment_getuniquename(bus), 1000));

    alljoyn_busattachment_destroy(bus);
}

TEST(BusAttachmentTest, ping_other_on_same_bus) {
    QStatus status;
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, alljoyn_busattachment_deletedefaultkeystore(s_busAttachmentTestName));

    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment otherbus = NULL;
    otherbus = alljoyn_busattachment_create(s_otherBusAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, alljoyn_busattachment_deletedefaultkeystore(s_otherBusAttachmentTestName));

    status = alljoyn_busattachment_start(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(otherbus, getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ASSERT_EQ(ER_OK, alljoyn_busattachment_ping(bus, alljoyn_busattachment_getuniquename(otherbus), 1000));

    status = alljoyn_busattachment_stop(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(otherbus);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
}

static QCC_BOOL AJ_CALL test_alljoyn_authlistener_requestcredentials(const void* context,
                                                                     AJ_PCSTR authMechanism,
                                                                     AJ_PCSTR peerName,
                                                                     uint16_t authCount,
                                                                     AJ_PCSTR userName,
                                                                     uint16_t credMask,
                                                                     alljoyn_credentials credentials)
{
    QCC_UNUSED(context);
    QCC_UNUSED(authMechanism);
    QCC_UNUSED(peerName);
    QCC_UNUSED(authCount);
    QCC_UNUSED(userName);
    QCC_UNUSED(credMask);
    QCC_UNUSED(credentials);
    return true;
}

static void AJ_CALL test_alljoyn_authlistener_authenticationcomplete(const void* context, AJ_PCSTR authMechanism, AJ_PCSTR peerName, QCC_BOOL success)
{

    QCC_UNUSED(authMechanism);
    QCC_UNUSED(peerName);
    QCC_UNUSED(success);
    if (context) {
        int* count = (int*) context;
        *count += 1;
    }
}


TEST(BusAttachmentTest, BasicSecureConnection)
{
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, alljoyn_busattachment_deletedefaultkeystore(s_busAttachmentTestName));
    ASSERT_EQ(ER_BUS_NOT_CONNECTED, alljoyn_busattachment_secureconnection(bus, "busname", false));

    QStatus status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(ER_BUS_NOT_CONNECTED, alljoyn_busattachment_secureconnection(bus, "busname", false));
    status = alljoyn_busattachment_connect(bus, getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(ER_BUS_SECURITY_NOT_ENABLED, alljoyn_busattachment_secureconnection(bus, "busname", false));

    alljoyn_busattachment otherbus = NULL;
    otherbus = alljoyn_busattachment_create(s_otherBusAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, alljoyn_busattachment_deletedefaultkeystore(s_otherBusAttachmentTestName));

    status = alljoyn_busattachment_start(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(otherbus, getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_authlistener al = NULL;
    alljoyn_authlistener_callbacks cbs;
    cbs.authentication_complete = &test_alljoyn_authlistener_authenticationcomplete;
    cbs.request_credentials = &test_alljoyn_authlistener_requestcredentials;
    cbs.security_violation = NULL;
    cbs.verify_credentials = NULL;
    al = alljoyn_authlistener_create(&cbs, NULL);

    EXPECT_EQ(ER_OK, alljoyn_busattachment_enablepeersecurity(bus, "ALLJOYN_ECDHE_NULL", al, "myKeyStore", true));
    EXPECT_EQ(ER_OK, alljoyn_busattachment_enablepeersecurity(otherbus, "ALLJOYN_ECDHE_NULL", al, "myOtherKeyStore", true));
    EXPECT_EQ(ER_OK, alljoyn_busattachment_secureconnection(bus, alljoyn_busattachment_getuniquename(otherbus), false));

    status = alljoyn_busattachment_stop(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_clearkeystore(otherbus);
    status = alljoyn_busattachment_join(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(otherbus);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_clearkeystore(bus);
    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
    alljoyn_authlistener_destroy(al);
}

TEST(BusAttachmentTest, BasicSecureConnectionAsync)
{
    alljoyn_busattachment bus = NULL;
    bus = alljoyn_busattachment_create(s_busAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, alljoyn_busattachment_deletedefaultkeystore(s_busAttachmentTestName));
    ASSERT_EQ(ER_BUS_NOT_CONNECTED, alljoyn_busattachment_secureconnectionasync(bus, "busname", false));

    QStatus status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(ER_BUS_NOT_CONNECTED, alljoyn_busattachment_secureconnectionasync(bus, "busname", false));
    status = alljoyn_busattachment_connect(bus, getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(ER_BUS_SECURITY_NOT_ENABLED, alljoyn_busattachment_secureconnectionasync(bus, "busname", false));

    alljoyn_busattachment otherbus = NULL;
    otherbus = alljoyn_busattachment_create(s_otherBusAttachmentTestName, QCC_TRUE);
    EXPECT_EQ(ER_OK, alljoyn_busattachment_deletedefaultkeystore(s_otherBusAttachmentTestName));

    status = alljoyn_busattachment_start(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(otherbus, getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_authlistener al = NULL;
    alljoyn_authlistener_callbacks cbs;
    cbs.authentication_complete = &test_alljoyn_authlistener_authenticationcomplete;
    cbs.request_credentials = &test_alljoyn_authlistener_requestcredentials;
    cbs.security_violation = NULL;
    cbs.verify_credentials = NULL;
    int authCompleteCount = 0;
    al = alljoyn_authlistener_create(&cbs, &authCompleteCount);

    EXPECT_EQ(ER_OK, alljoyn_busattachment_enablepeersecurity(bus, "ALLJOYN_ECDHE_NULL", al, "myKeyStore", true));
    EXPECT_EQ(ER_OK, alljoyn_busattachment_enablepeersecurity(otherbus, "ALLJOYN_ECDHE_NULL", al, "myOtherKeyStore", true));
    EXPECT_EQ(ER_OK, alljoyn_busattachment_secureconnectionasync(bus, alljoyn_busattachment_getuniquename(otherbus), false));

    int ticks = 0;
    while ((authCompleteCount < 2) && (ticks++ < 50)) {
        qcc::Sleep(100);
    }
    EXPECT_EQ(2, authCompleteCount);
    status = alljoyn_busattachment_stop(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_clearkeystore(otherbus);
    status = alljoyn_busattachment_join(otherbus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(otherbus);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_busattachment_clearkeystore(bus);
    status = alljoyn_busattachment_join(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_busattachment_destroy(bus);
    alljoyn_authlistener_destroy(al);
}
