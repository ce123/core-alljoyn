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

package org.alljoyn.config.samples.configclient.security;

/**
 * An API between the {@link SrpAnonymousKeyListener} and the application.
 * The application will implement a password handler.
 *
 * When the bus requires authentication with a remote device, it will let the password handler
 * (the application) handle it. When the bus receives a result of an authentication attempt with
 * a remote device, it will let the password handler (the application) handle it.
 */
public interface AuthPasswordHandler {
	/**
	 * Get the password for this peer. The application should store a password per peer
	 * @param peerName the alljoyn bus name of the peer we want to authenticate with
	 * @return the password for this peer
	 */
	public char[] getPassword(String peerName);

	/**
	 * Once the authentication has finished the completed(...) call-back method is called.
	 * The application can then show a popup, a toast, etc.
	 * @param mechanism The authentication mechanism that was just completed
	 * @param authPeer The peerName (well-known name or unique name)
	 * @param authenticated A boolean variable indicating if the authentication attempt completed successfully.
	 */
	public void completed(String mechanism, String authPeer, boolean authenticated);
}