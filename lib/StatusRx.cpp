/*
 * Copyright (c) 2017-2020 Aaron Marburg <amarburg@uw.edu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of University of Washington nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <sstream>

#include <arpa/inet.h>

#include <boost/bind.hpp>

#include "liboculus/StatusRx.h"
#include "g3log/g3log.hpp"

namespace liboculus {

  using std::string;

  using boost::asio::ip::address_v4;

  // ----------------------------------------------------------------------------
  // StatusRx - a listening socket for oculus status messages

  StatusRx::StatusRx(boost::asio::io_service &context )
    : _status(),
    _port( 52102 ),
    _valid(0), _invalid(0),
    _ioService(context),
    _socket(_ioService),
    _inputBuffer( sizeof(OculusStatusMsg) ),
    _deadline(_ioService),
    _sonarStatusCallback()
  {
    doConnect();
  }

  StatusRx::~StatusRx(){
    ;
  }

  void StatusRx::doConnect()
  {
  boost::asio::ip::udp::endpoint local( boost::asio::ip::address_v4::any(), 52102);
    boost::system::error_code error;

    _socket.open(boost::asio::ip::udp::v4(), error);

    boost::asio::socket_base::broadcast option(true);
    _socket.set_option(option);

    if(!error) {
      _socket.bind(local);

      startReader();
    } else {
      LOG(WARNING) << "Unable to start reader";
    }
  }

  void StatusRx::startReader()
  {
    // Set a deadline for the read operation.
    //deadline_.expires_from_now(boost::posix_time::seconds(30));

    // Start an asynchronous receive
    _socket.async_receive( boost::asio::buffer((void *)&_osm, sizeof(OculusStatusMsg)), boost::bind(&StatusRx::handleRead, this, _1, _2));
  }

  void StatusRx::handleRead(const boost::system::error_code& ec, std::size_t bytes_transferred )
  {
    if (!ec) {
      // Extract the newline-delimited message from the buffer.

      if( bytes_transferred != sizeof(OculusStatusMsg)) {
        LOG(WARNING) << "Got " << bytes_transferred << " bytes, expected OculusStatusMsg of size " << sizeof(OculusStatusMsg);
        return;
      }

      LOG(DEBUG) << "Got status message.  Updating!";
      _status.update( _osm );
      if( _sonarStatusCallback ) _sonarStatusCallback( _status );

      _valid++;

      // Schedule another read
      startReader();
    }
    else
    {
      LOG(WARNING) << "Error on receive: " << ec.message();

      //stop();
    }
  }


}
