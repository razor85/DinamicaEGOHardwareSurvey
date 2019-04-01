#include <cstdint>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <curl/curl.h>

#include "curlPostInfo.h"


namespace CurlPost {


struct CurlMemory {
  std::vector< uint8_t > data;
};

size_t writeCallback( void *contents, size_t size, size_t nmemb, void* ptr ) {
  const size_t extraSize { size * nmemb };
  CurlMemory* memoryPtr = reinterpret_cast< CurlMemory* >( ptr );

  const size_t oldSize = memoryPtr->data.size();
  if ( extraSize > 0 )
    memoryPtr->data.resize( oldSize + extraSize + 1 );

  memcpy( &memoryPtr->data[ oldSize ], contents, extraSize );
  memoryPtr->data[ oldSize + extraSize ] = 0;

  return extraSize;
}

bool isTransferOk( const CurlMemory& data ) {
  std::string transferAnswer( ( char* ) data.data.data() );
  boost::algorithm::trim( transferAnswer );

  return transferAnswer == "OK";
}

bool sendPost( const std::string& postData ) {
  CurlMemory transferMemory;
  const char* postDataPtr = postData.c_str();

  curl_global_init( CURL_GLOBAL_ALL );
  CURL* curlPtr = curl_easy_init();
  if ( curlPtr ) {
    curl_easy_setopt( curlPtr, CURLOPT_URL, "POST URL" );
    curl_easy_setopt( curlPtr, CURLOPT_WRITEFUNCTION, writeCallback );
    curl_easy_setopt( curlPtr, CURLOPT_WRITEDATA, ( void* ) &transferMemory );
    curl_easy_setopt( curlPtr, CURLOPT_POSTFIELDS, postDataPtr );
    curl_easy_setopt( curlPtr, CURLOPT_POSTFIELDSIZE, ( long ) postData.size() );
    
    CURLcode performResult = curl_easy_perform( curlPtr );
    curl_easy_cleanup( curlPtr );
    curl_global_cleanup();

    return performResult == CURLE_OK &&
      isTransferOk( transferMemory );
  }

  return false;
}


} // namespace CurlPost
