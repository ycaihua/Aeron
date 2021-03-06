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

#include <array>

#include <gtest/gtest.h>

#include <mintomic/mintomic.h>

#include <thread>
#include "MockAtomicBuffer.h"
#include <concurrent/broadcast/BroadcastBufferDescriptor.h>
#include <concurrent/broadcast/BroadcastTransmitter.h>

using namespace aeron::common::concurrent::broadcast;
using namespace aeron::common::concurrent::mock;
using namespace aeron::common::concurrent;
using namespace aeron::common;

#define CAPACITY (1024)
#define TOTAL_BUFFER_SIZE (CAPACITY + BroadcastBufferDescriptor::TRAILER_LENGTH)
#define SRC_BUFFER_SIZE (1024)
#define MSG_TYPE_ID (7)
#define TAIL_COUNTER_INDEX (CAPACITY + BroadcastBufferDescriptor::TAIL_COUNTER_OFFSET)
#define LATEST_COUNTER_INDEX (CAPACITY + BroadcastBufferDescriptor::LATEST_COUNTER_OFFSET)

typedef std::array<std::uint8_t, TOTAL_BUFFER_SIZE> buffer_t;
typedef std::array<std::uint8_t, SRC_BUFFER_SIZE> src_buffer_t;

class BroadcastTransmitterTest : public testing::Test
{
public:
    BroadcastTransmitterTest() :
        m_mockBuffer(&m_buffer[0], m_buffer.size()),
        m_broadcastTransmitter(m_mockBuffer)
    {
        m_buffer.fill(0);
    }

    virtual void SetUp()
    {
        m_buffer.fill(0);
    }

protected:
    MINT_DECL_ALIGNED(buffer_t m_buffer, 16);
    MockAtomicBuffer m_mockBuffer;
    BroadcastTransmitter m_broadcastTransmitter;
};

TEST_F(BroadcastTransmitterTest, shouldCalculateCapacityForBuffer)
{
    EXPECT_EQ(m_broadcastTransmitter.capacity(), CAPACITY);
}

TEST_F(BroadcastTransmitterTest, shouldThrowExceptionForCapacityThatIsNotPowerOfTwo)
{
    typedef std::array<std::uint8_t, (777 + BroadcastBufferDescriptor::TRAILER_LENGTH)> non_power_of_two_buffer_t;
    MINT_DECL_ALIGNED(non_power_of_two_buffer_t non_power_of_two_buffer, 16);
    AtomicBuffer buffer(&non_power_of_two_buffer[0], non_power_of_two_buffer.size());

    ASSERT_THROW(
    {
        BroadcastTransmitter transmitter(buffer);
    }, util::IllegalStateException);
}

TEST_F(BroadcastTransmitterTest, shouldThrowExceptionWhenMaxMessageSizeExceeded)
{
    MINT_DECL_ALIGNED(src_buffer_t buffer, 16);
    AtomicBuffer srcBuffer(&buffer[0], buffer.size());

    ASSERT_THROW(
    {
        m_broadcastTransmitter.transmit(MSG_TYPE_ID, srcBuffer, 0, m_broadcastTransmitter.maxMsgLength() + 1);
    }, util::IllegalArgumentException);
}

TEST_F(BroadcastTransmitterTest, shouldThrowExceptionWhenMessageTypeIdInvalid)
{
    MINT_DECL_ALIGNED(src_buffer_t buffer, 16);
    AtomicBuffer srcBuffer(&buffer[0], buffer.size());
    const std::int32_t invalidMsgTypeId = -1;

    ASSERT_THROW(
    {
        m_broadcastTransmitter.transmit(invalidMsgTypeId, srcBuffer, 0, 32);
    }, util::IllegalArgumentException);
}

TEST_F(BroadcastTransmitterTest, shouldTransmitIntoEmptyBuffer)
{
    MINT_DECL_ALIGNED(src_buffer_t buffer, 16);
    AtomicBuffer srcBuffer(&buffer[0], buffer.size());
    const std::int64_t tail = 0;
    const std::int32_t recordOffset = (std::int32_t)tail;
    const std::int32_t length = 8;
    const std::int32_t recordLength = util::BitUtil::align(length + RecordDescriptor::HEADER_LENGTH, RecordDescriptor::RECORD_ALIGNMENT);
    const util::index_t srcIndex = 0;
    testing::Sequence sequence;

    EXPECT_CALL(m_mockBuffer, getInt64(TAIL_COUNTER_INDEX))
        .Times(1)
        .InSequence(sequence)
        .WillOnce(testing::Return(tail));
    EXPECT_CALL(m_mockBuffer, putInt64Ordered(RecordDescriptor::tailSequenceOffset(recordOffset), tail))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putInt32(RecordDescriptor::recLengthOffset(recordOffset), recordLength))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putInt32(RecordDescriptor::msgLengthOffset(recordOffset), length))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putInt32(RecordDescriptor::msgTypeOffset(recordOffset), MSG_TYPE_ID))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putBytes(RecordDescriptor::msgOffset(recordOffset), testing::Ref(srcBuffer), srcIndex, length))
        .Times(1)
        .InSequence(sequence);

    EXPECT_CALL(m_mockBuffer, putInt64(LATEST_COUNTER_INDEX, tail))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putInt64Ordered(TAIL_COUNTER_INDEX, tail + recordLength))
        .Times(1)
        .InSequence(sequence);

    m_broadcastTransmitter.transmit(MSG_TYPE_ID, srcBuffer, srcIndex, length);
}

TEST_F(BroadcastTransmitterTest, shouldTransmitIntoUsedBuffer)
{
    MINT_DECL_ALIGNED(src_buffer_t buffer, 16);
    AtomicBuffer srcBuffer(&buffer[0], buffer.size());
    const std::int64_t tail = RecordDescriptor::RECORD_ALIGNMENT * 3;
    const std::int32_t recordOffset = (std::int32_t)tail;
    const std::int32_t length = 8;
    const std::int32_t recordLength = util::BitUtil::align(length + RecordDescriptor::HEADER_LENGTH, RecordDescriptor::RECORD_ALIGNMENT);
    const util::index_t srcIndex = 0;
    testing::Sequence sequence;

    EXPECT_CALL(m_mockBuffer, getInt64(TAIL_COUNTER_INDEX))
        .Times(1)
        .InSequence(sequence)
        .WillOnce(testing::Return(tail));
    EXPECT_CALL(m_mockBuffer, putInt64Ordered(RecordDescriptor::tailSequenceOffset(recordOffset), tail))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putInt32(RecordDescriptor::recLengthOffset(recordOffset), recordLength))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putInt32(RecordDescriptor::msgLengthOffset(recordOffset), length))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putInt32(RecordDescriptor::msgTypeOffset(recordOffset), MSG_TYPE_ID))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putBytes(RecordDescriptor::msgOffset(recordOffset), testing::Ref(srcBuffer), srcIndex, length))
        .Times(1)
        .InSequence(sequence);

    EXPECT_CALL(m_mockBuffer, putInt64(LATEST_COUNTER_INDEX, tail))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putInt64Ordered(TAIL_COUNTER_INDEX, tail + recordLength))
        .Times(1)
        .InSequence(sequence);

    m_broadcastTransmitter.transmit(MSG_TYPE_ID, srcBuffer, srcIndex, length);
}

TEST_F(BroadcastTransmitterTest, shouldTransmitIntoEndOfBuffer)
{
    MINT_DECL_ALIGNED(src_buffer_t buffer, 16);
    AtomicBuffer srcBuffer(&buffer[0], buffer.size());
    const std::int64_t tail = CAPACITY - RecordDescriptor::RECORD_ALIGNMENT;
    const std::int32_t recordOffset = (std::int32_t)tail;
    const std::int32_t length = 8;
    const std::int32_t recordLength = util::BitUtil::align(length + RecordDescriptor::HEADER_LENGTH, RecordDescriptor::RECORD_ALIGNMENT);
    const util::index_t srcIndex = 0;
    testing::Sequence sequence;

    EXPECT_CALL(m_mockBuffer, getInt64(TAIL_COUNTER_INDEX))
        .Times(1)
        .InSequence(sequence)
        .WillOnce(testing::Return(tail));
    EXPECT_CALL(m_mockBuffer, putInt64Ordered(RecordDescriptor::tailSequenceOffset(recordOffset), tail))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putInt32(RecordDescriptor::recLengthOffset(recordOffset), recordLength))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putInt32(RecordDescriptor::msgLengthOffset(recordOffset), length))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putInt32(RecordDescriptor::msgTypeOffset(recordOffset), MSG_TYPE_ID))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putBytes(RecordDescriptor::msgOffset(recordOffset), testing::Ref(srcBuffer), srcIndex, length))
        .Times(1)
        .InSequence(sequence);

    EXPECT_CALL(m_mockBuffer, putInt64(LATEST_COUNTER_INDEX, tail))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putInt64Ordered(TAIL_COUNTER_INDEX, tail + recordLength))
        .Times(1)
        .InSequence(sequence);

    m_broadcastTransmitter.transmit(MSG_TYPE_ID, srcBuffer, srcIndex, length);
}

TEST_F(BroadcastTransmitterTest, shouldApplyPaddingWhenInsufficientSpaceAtEndOfBuffer)
{
    MINT_DECL_ALIGNED(src_buffer_t buffer, 16);
    AtomicBuffer srcBuffer(&buffer[0], buffer.size());
    std::int64_t tail = CAPACITY - RecordDescriptor::RECORD_ALIGNMENT;
    std::int32_t recordOffset = (std::int32_t)tail;
    const std::int32_t length = RecordDescriptor::RECORD_ALIGNMENT + 8;
    const std::int32_t recordLength = util::BitUtil::align(length + RecordDescriptor::HEADER_LENGTH, RecordDescriptor::RECORD_ALIGNMENT);
    const util::index_t srcIndex = 0;
    testing::Sequence sequence;

    EXPECT_CALL(m_mockBuffer, getInt64(TAIL_COUNTER_INDEX))
        .Times(1)
        .InSequence(sequence)
        .WillOnce(testing::Return(tail));

    EXPECT_CALL(m_mockBuffer, putInt64Ordered(RecordDescriptor::tailSequenceOffset(recordOffset), tail))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putInt32(RecordDescriptor::recLengthOffset(recordOffset), CAPACITY - recordOffset))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putInt32(RecordDescriptor::msgLengthOffset(recordOffset), 0))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putInt32(RecordDescriptor::msgTypeOffset(recordOffset), RecordDescriptor::PADDING_MSG_TYPE_ID))
        .Times(1)
        .InSequence(sequence);

    tail += (CAPACITY - recordOffset);
    recordOffset = 0;

    EXPECT_CALL(m_mockBuffer, putInt64Ordered(RecordDescriptor::tailSequenceOffset(recordOffset), tail))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putInt32(RecordDescriptor::recLengthOffset(recordOffset), recordLength))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putInt32(RecordDescriptor::msgLengthOffset(recordOffset), length))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putInt32(RecordDescriptor::msgTypeOffset(recordOffset), MSG_TYPE_ID))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putBytes(RecordDescriptor::msgOffset(recordOffset), testing::Ref(srcBuffer), srcIndex, length))
        .Times(1)
        .InSequence(sequence);

    EXPECT_CALL(m_mockBuffer, putInt64(LATEST_COUNTER_INDEX, tail))
        .Times(1)
        .InSequence(sequence);
    EXPECT_CALL(m_mockBuffer, putInt64Ordered(TAIL_COUNTER_INDEX, tail + recordLength))
        .Times(1)
        .InSequence(sequence);

    m_broadcastTransmitter.transmit(MSG_TYPE_ID, srcBuffer, srcIndex, length);
}