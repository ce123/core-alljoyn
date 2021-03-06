/*
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
*/

package org.alljoyn.bus.annotation;

import java.lang.annotation.Documented;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Indicates that a particular member of an AllJoyn exportable interface
 * is defined to be a AllJoyn signal.
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.METHOD)
public @interface BusSignal {

    /**
     * Override of signal name.
     * The default AllJoyn signal name is the Java method name.
     *
     * @return name specified in the BusSignal annotation
     */
    String name() default "";

    /**
     * Input signature for signal.
     *
     * @see Signature
     *
     * @return signature specified in the BusSignal annotation
     */
    String signature() default "";

    /**
     * Output signature for signal.
     *
     * @see Signature
     *
     * @return replySignature specified in the BusSignal annotation
     */
    String replySignature() default "";

    /**
     * Description for this signal
     *
     * @return description specified in the BusSignal annotation
     */
    String description() default "";

    /**
     * Set to true to document that this signal will be sent sessionless.
     * Note that this does not cause the signal to be sent sessionless,
     * just documents it as such.
     *
     * @return sessionless indicator specified in the BusSignal annotation
     */
    boolean sessionless() default false;

    /** Deprecated annotate flag. */
    int ANNOTATE_DEPRECATED = 2;

    /** Sessioncast annotate flag. */
    int ANNOTATE_SESSIONCAST = 4;

    /** Sessionless annotate flag. */
    int ANNOTATE_SESSIONLESS = 8;

    /** Unicast annotate flag. */
    int ANNOTATE_UNICAST = 16;

    /** Global broadcast annotate flag. */
    int ANNOTATE_GLOBAL_BROADCAST = 32;

    /**
     * Annotate introspection data for method.
     * The annotation may be the flag ANNOTATE_DEPRECATED (see
     * org.freedesktop.DBus.Deprecated in the D-Bus Specification),
     * and/or any of the flags ANNOTATE_SESSIONCAST, ANNOTATE_SESSIONLESS,
     * ANNOTATE_UNICAST, or ANNOTATE_GLOBAL_BROADCAST.
     *
     * @return annotation specifying the signal behavior, and also whether
     * this signal is deprecated according to the BusSignal annotation
     */
    int annotation() default 0;
}
