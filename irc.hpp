#ifndef IRC_HPP
#define IRC_HPP

#include <boost/asio.hpp>
#include <functional>
#include <fmt/core.h>
#include <fmt/color.h>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace asio = boost::asio;
namespace sys = boost::system;
namespace ip = boost::asio::ip;

namespace irc
{
struct Command
{
	//! The command prefix, non-empty if present.
	std::string prefix;

	//! Name for the command, converted to lowercase, e.g. "privmsg".
	std::string name;

	//! Parameters passed through the command.
	std::vector<std::string> parameters;

	Command() = default;
	Command(std::string p_name, std::vector<std::string> p_parameters = {});
	Command(std::string p_name, std::string p_single_parameter);
};

class Client
{
	asio::io_service& io;
	asio::streambuf buf;
	ip::tcp::resolver resolver{io};
	ip::tcp::socket socket{io};

	//! Setup the command dispatcher, which handles some internal work required to keep the IRC connection functional (such as PONG)
	//! and feeds the parsed command to the user-specified on_recv.
	//! Do note that even though some commands like PING are handled internally, they are still passed to on_recv.
	void setup_command_dispatcher();

	void internal_command_dispatch(const Command& raw_command);

	void handle_ctcp(const Command& cmd);

public:
	Client(asio::io_service& p_io);

	static std::string_view parse_username(std::string_view prefix);

	std::string
	    server,
	    port = "6667",
	    nickname = "asubot",
	    username = nickname,
	    realname = nickname;

	using EventCallback = std::function<void(const Command&)>;
	static constexpr auto noop_event_handler = [](const Command&) {};

	EventCallback
	    on_auth = noop_event_handler,
	    on_recv = noop_event_handler,
	    on_send = noop_event_handler;

	void command(Command cmd);

	void initialize();
};
}

#endif // IRC_HPP
