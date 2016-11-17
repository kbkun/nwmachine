#ifndef HELPERS_H
#define HELPERS_H

#include <QSharedPointer>
#include <stdexcept>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>

/*!
 * \brief Расшифровывает сообщение, предваренное длиной из массива байтов.
 *
 * \tparam Message Тип сообщения, для работы с которым предназначен данный класс.
 *
 * \param data Указатель на буфер данных.
 * \param size Размер буфера данных.
 * \param bytesConsumed Количество байт, которое потребовалось для расшифровки сообщения в случае успеха.
 *
 * \return Умный указатель на сообщение. Если удалось расшифровать сообщение, то он не пустой.
 */
template<typename Message>
const QSharedPointer<Message> parseDelimited(const void* data, size_t size, size_t& bytesConsumed)
{
  google::protobuf::io::CodedInputStream is(reinterpret_cast<const google::protobuf::uint8*>(data), size);

  google::protobuf::uint32 messageSize = 0;
  const bool isMessageSizeRead = is.ReadVarint32(&messageSize);
  if(!isMessageSizeRead)
  {
    return QSharedPointer<Message>();
  }

  const size_t totalFrameSize = google::protobuf::io::CodedOutputStream::VarintSize32(messageSize) + messageSize;

  if (size < totalFrameSize)
  {
    return QSharedPointer<Message>();
  }

  const QSharedPointer<Message> result(new Message);
  is.PushLimit(messageSize);
  const bool parseOk = result->ParseFromCodedStream(&is);
  if (!parseOk)
  {
    if (!result->IsInitialized())
    {
      throw std::runtime_error("the message of type \"" + result->GetTypeName() + "\" is missing required fields: "
        + result->InitializationErrorString());
    }
    else
    {
      throw std::runtime_error("the message of type \"" + result->GetTypeName() + "\" a strange error");
    }
  }

  bytesConsumed = totalFrameSize;

  return result;
}

template <typename Message> const QByteArray serializeDelimited(const Message& msg)
{
  const size_t messageSize = msg.ByteSize();
  const size_t headerSize = google::protobuf::io::CodedOutputStream::VarintSize32(messageSize);
  const size_t totalFrameSize = messageSize + headerSize;

  QByteArray result(totalFrameSize, 0);

  google::protobuf::uint8* buffer = reinterpret_cast<google::protobuf::uint8*>(result.data());

  google::protobuf::io::CodedOutputStream::WriteVarint32ToArray(messageSize, buffer);
  msg.SerializeWithCachedSizesToArray(buffer + headerSize);

  return result;
}

#endif // HELPERS_H
