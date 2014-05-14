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
package uk.co.real_logic.aeron.mediadriver.buffer;

import uk.co.real_logic.aeron.mediadriver.UdpDestination;

/**
 * Interface for encapsulating the allocation of ByteBuffers for Session, Channel, and Term
 */
public interface BufferManagement extends AutoCloseable
{
    static BufferManagement newMappedBufferManager(final String dataDir)
    {
        return new MappedBufferManagement(dataDir);
    }

    BufferRotator addPublisherChannel(final UdpDestination destination,
                                      final long sessionId,
                                      final long channelId) throws Exception;

    void removePublisherChannel(final UdpDestination destination,
                                final long sessionId,
                                final long channelId) throws IllegalArgumentException;

    BufferRotator addSubscriberChannel(final UdpDestination destination,
                                       final long sessionId,
                                       final long channelId) throws Exception;

    void removeSubscriberChannel(final UdpDestination destination,
                                 final long sessionId,
                                 final long channelId) throws IllegalArgumentException;

}