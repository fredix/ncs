/**@file
 * @brief      CFtpServer - Internal configuration
 *
 * @author     Mail :: thebrowser@gmail.com
 *
 * @copyright  Copyright (C) 2007 Julien Poumailloux
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 * 4. If you like this software, a fee would be appreciated, particularly if
 *    you use this software in a commercial product, but is not required.
 */

/*
 * MODIFIED by frederic@logier.org for NCS
 * REMOVED useless win32 directive
 */


#define SOCKET_ERROR	-1
#define INVALID_SOCKET	-1
#define MAX_PATH PATH_MAX
#ifndef O_BINARY
    #define O_BINARY	0
#endif
#define CloseSocket(x)	(shutdown((x), SHUT_RDWR), close(x))

#ifndef MSG_NOSIGNAL
	#ifdef SO_NOSIGPIPE
		#define MSG_NOSIGNAL	SO_NOSIGPIPE
	#else
		#define MSG_NOSIGNAL	0
	#endif
#endif

// Internal settings. Do not modify!
#define CFTPSERVER_REPLY_MAX_LEN		(MAX_PATH + 64)
#define CFTPSERVER_LIST_MAX_LINE_LEN	(MAX_PATH + 57)
