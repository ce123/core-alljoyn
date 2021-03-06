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

package org.alljoyn.bus.ifaces;

import org.alljoyn.bus.BusException;
import org.alljoyn.bus.annotation.BusInterface;
import org.alljoyn.bus.annotation.BusMethod;
import org.alljoyn.bus.annotation.BusProperty;

/**
 * Definition of the Icon BusInterface
 */
@BusInterface (name = Icon.INTERFACE_NAME, announced="true")
public interface Icon
{
    public static final String INTERFACE_NAME = "org.alljoyn.Icon";
    public final static String OBJ_PATH = "/About/DeviceIcon";

    /**
     * @return Interface version
     * @throws BusException indicating failure to retrieve Version property
     */
    @BusProperty(signature="q")
    short getVersion() throws BusException;

    /**
     * @return Mime type for the icon
     * @throws BusException indicating failure to retrieve MimeType property
     */
    @BusProperty(signature="s")
    String getMimeType() throws BusException;

    /**
     * @return Size of the icon
     * @throws BusException indicating failure to retrieve Size property
     */
    @BusProperty(signature="u")
    int getSize() throws BusException;

    /**
     * Returns the URL if the icon is hosted on the cloud
     * @return the URL if the icon is hosted on the cloud
     * @throws BusException indicating failure to make GetUrl method call
     */
    @BusMethod(name="GetUrl", replySignature="s")
    String getUrl() throws BusException;

    /**
     * Returns binary content for the icon
     * @return binary content for the icon
     * @throws BusException indicating failure to make GetContent method call
     */
    @BusMethod(name="GetContent", replySignature="ay")
    byte[] getContent() throws BusException;
}
