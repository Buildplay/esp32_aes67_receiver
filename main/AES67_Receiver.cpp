#include <Arduino.h>
#include "AES67_Receiver.hpp"

AES67_Receiver::AES67_Receiver(
   unsigned int bufferSize
)
{
   _maxBufferSize = bufferSize;
   _maskSize = _maxBufferSize - 1;

   if (bufferSize > 0 and (bufferSize & (bufferSize - 1)) == 0) {
      _isPowerOfTwo = true;
   } else {
      _isPowerOfTwo = false;
   }

   _buffer = (String *)calloc(_maxBufferSize, sizeof(String));
   _headIndex = 0;
   _tailIndex = 0;
};

AES67_Receiver::~AES67_Receiver() {
   free(_buffer);
};

bool AES67_Receiver::push(String str)
{
   return _isPowerOfTwo ? doPushPower(str) : doPushNotPower(str);
};

String AES67_Receiver::unshift()
{
   return _isPowerOfTwo ? doUnshiftPower() : doUnshiftNotPower();
};

bool AES67_Receiver::doPushPower(String str)
{
   _buffer[_tailIndex] = str;
   _tailIndex = (_tailIndex + 1) & _maskSize;
   return true;
};
bool AES67_Receiver::doPushNotPower(String str)
{
   _buffer[_tailIndex] = str;
   _tailIndex = (_tailIndex + 1) % _maskSize;
   return true;
};

String AES67_Receiver::doUnshiftPower() {
   String str(_buffer[_headIndex]);
   _headIndex = (_headIndex + 1) & _maskSize;
   return str;
};
String AES67_Receiver::doUnshiftNotPower()
{
   String str(_buffer[_headIndex]);
   _headIndex = (_headIndex + 1) % _maskSize;
   return str;
};