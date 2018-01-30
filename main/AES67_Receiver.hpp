#ifndef AES67_RECEIVER_HPP

#define AES67_RECEIVER_HPP

#include <Arduino.h>
#include "ex_string.hpp"

class AES67_Receiver
{
 private:
   static const unsigned int MAX_BUFFER_SIZE;

   unsigned int _maxBufferSize;
   unsigned int _maskSize;
   bool _isPowerOfTwo;
   String *_buffer;
   unsigned int _headIndex;
   unsigned int _tailIndex;

   bool doPushPower(String str);
   bool doPushNotPower(String str);

   String doUnshiftPower();
   String doUnshiftNotPower();

 public:
   AES67_Receiver(unsigned int bufferSize);
   ~AES67_Receiver();

   String parse(String payload);
   bool checkValid();
   void parseAndBuffer(String payload);

   inline unsigned int currentBufferLength(); // inlined
   bool push(String str);
   String unshift();

   bool isPowerOfTwo() { return _isPowerOfTwo; };  // for debug
};

unsigned int AES67_Receiver::currentBufferLength()
{
   int len = _tailIndex - _headIndex;
   if (len >= 0)
   {
      return len + 1;
   };
   return len + 1 + _maxBufferSize;
};

#endif