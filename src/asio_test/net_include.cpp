#pragma once
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

#ifdef _WIN32
#define _WIN32_WINNT 0x0a00
#endif
#define ASIO_STANDALONE
#include <asio-1.22.2\include\asio.hpp>
#include <asio-1.22.2\include\asio\ts\buffer.hpp>
#include <asio-1.22.2\include\asio\ts\internet.hpp>