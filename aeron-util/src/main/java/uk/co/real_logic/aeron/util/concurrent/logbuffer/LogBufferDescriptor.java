/*
 * Copyright 2014 Real Logic Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package uk.co.real_logic.aeron.util.concurrent.logbuffer;

import uk.co.real_logic.aeron.util.BitUtil;
import uk.co.real_logic.aeron.util.concurrent.AtomicBuffer;

import static java.lang.Integer.valueOf;
import static uk.co.real_logic.aeron.util.concurrent.logbuffer.RecordDescriptor.RECORD_ALIGNMENT;

/**
 * Layout description for the log buffer state.
 */
public class LogBufferDescriptor
{
    /** Offset within the trailer where the tail value is stored. */
    public static final int TAIL_COUNTER_OFFSET;

    /** Total size of the state buffer */
    public static final int STATE_SIZE;

    static
    {
        int offset = 0;
        TAIL_COUNTER_OFFSET = offset;

        offset += BitUtil.CACHE_LINE_SIZE;
        STATE_SIZE = offset;
    }

    /** Minimum buffer size for the log */
    public static final int LOG_MIN_SIZE = RecordDescriptor.RECORD_ALIGNMENT * 256;

    /**
     * Check that log buffer is the correct size and alignment.
     *
     * @param buffer to be checked.
     * @throws IllegalStateException if the buffer is not as expected.
     */
    public static void checkLogBuffer(final AtomicBuffer buffer)
    {
        final int capacity = buffer.capacity();
        if (capacity < LOG_MIN_SIZE)
        {
            final String s = String.format("Log buffer capacity less than min size of %d, capacity=%d",
                                           valueOf(LOG_MIN_SIZE), valueOf(capacity));
            throw new IllegalStateException(s);
        }

        if (capacity % RECORD_ALIGNMENT != 0)
        {
            final String s = String.format("Log buffer capacity not a multiple of %d, capacity=%d",
                                           valueOf(RECORD_ALIGNMENT), valueOf(capacity));
            throw new IllegalStateException(s);
        }
    }

    /**
     * Check that state buffer is of sufficient size.
     *
     * @param buffer to be checked.
     * @throws IllegalStateException if the buffer is not as expected.
     */
    public static void checkStateBuffer(final AtomicBuffer buffer)
    {
        final int capacity = buffer.capacity();
        if (capacity < STATE_SIZE)
        {
            final String s = String.format("State buffer capacity less than min size of %d, capacity=%d",
                                           valueOf(STATE_SIZE), valueOf(capacity));
            throw new IllegalStateException(s);
        }
    }
}