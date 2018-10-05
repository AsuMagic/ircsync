#include "irc.hpp"
#include <array>
#include <memory>
#include <vector>

struct ServerConnection
{
	std::string_view name;
	std::string_view server, port;
	std::vector<std::string> channels;
	std::unique_ptr<irc::Client> client = {};
};

bool operator<(const ServerConnection& a, const ServerConnection& b)
{
	return std::tie(a.server, a.port) < std::tie(b.server, b.port);
}

struct User
{
	std::string_view nickname, hostname;
};

bool operator==(const User& a, const User& b)
{
	return a.nickname == b.nickname
		|| a.hostname == b.hostname;
}

std::array<ServerConnection, 1> connections {{
	//{"Freenode", "irc.freenode.net", "6667", {"#CBNA"} },
	{"Mibbit",   "irc.mibbit.net",   "6667", {"#ircsync2", "#ircsync"} },
}};

std::vector<User> users;

std::array<std::string_view, 3> whitelisted_domains {{
	"pastebin.com",
	"imgur.com",
	"wyvup.com",
}};

void setup_connection(ServerConnection& c)
{
	//! Inserts a character within usernames to prevent unintentional pinging under some IRC clients.
	constexpr bool obfuscate_username = true;

	//! Omits the server and channel from forwarded messages.
	constexpr bool omit_server_channel = true;

	//! Sends a message upon joining a server.
	constexpr bool greeting = true;

	irc::Client& irc = *c.client;

	irc.nickname = "ircsync";
	irc.server = c.server;
	irc.port = c.port;

	irc.on_auth = [&c = c, &irc = irc](const irc::Command&) {
		for (const auto& channel : c.channels)
		{
			irc.command({"join", channel});

			if (greeting)
			{
				irc.command({"privmsg", {channel, std::string{"Now syncing "} + channel}});
			}
		}
	};

	auto print_command = [&c = c](const irc::Command& cmd, const auto direction_string) {
		fmt::print(fmt::color::light_blue, "[{}] ", c.name);

		if (!cmd.prefix.empty())
		{
			fmt::print(fmt::color::gray, "{} ", cmd.prefix);
		}

		fmt::print(fmt::color::gray, "{}", direction_string);
		fmt::print(fmt::color::orange, "{} ", cmd.name);
		fmt::print("{}\n", fmt::join(cmd.parameters, " "));
	};

	irc.on_recv = [=, &c = c, &irc = irc](const irc::Command& cmd) {
		print_command(cmd, "<<< ");

		if (cmd.name == "privmsg" && cmd.parameters.size() == 2)
		{
			std::string user{irc::Client::parse_username(cmd.prefix)};
			std::string_view source_chan = cmd.parameters[0];
			std::string_view message = cmd.parameters[1];

			if (!source_chan.empty() && source_chan[0] == '#')
			{
				if (message == ".servers")
				{
					std::string composed = "Syncs ";

					// TODO: make servers printable somehow to clean this up
					for (auto& nc : connections)
					{
						composed += std::string{nc.server} + ':' + std::string{nc.port} + " (" + fmt::format("{}", fmt::join(nc.channels, ", ")) + ") ";
					}

					irc.command({"privmsg", {std::string{source_chan}, composed}});
					return;
				}
				else if (message == ".help")
				{
					irc.command({"privmsg", {user, "IRCSync - Synchronize messages across channels and servers"}});
					irc.command({"privmsg", {user, "Repository: https://github.com/AsuMagic/ircsync"}});
					irc.command({"privmsg", {user, "Contact me: Asu at irc.freenode.net"}});
				}

				if (obfuscate_username)
				{
					user.insert(user.begin() + 1, '_');
				}

				for (auto& nc : connections)
				for (auto& chan : nc.channels)
				{
					bool same_chan = (nc.server == c.server) && (chan == source_chan);

					if (!same_chan)
					{
						std::string composed;

						// Bypass checks when asked
						if (omit_server_channel)
						{
							composed = fmt::format("{}: {}", user, message);
						}
						// Omit channel name when on different servers but with the same channel name
						else if (chan == source_chan)
						{
							composed = fmt::format("[{}] {}: {}", c.name, user, message);
						}
						// Omit server name when on the same server
						else if (nc.server == c.server)
						{
							composed = fmt::format("[{}] {}: {}", source_chan, user, message);
						}
						// Most verbose output: Different server, different channel name
						else
						{
							composed = fmt::format("[{} {}] {}: {}", c.name, source_chan, user, message);
						}

						nc.client->command({"privmsg", {chan, composed}});
					}
				}
			}
		}
	};

	irc.on_send = [=](const irc::Command& cmd) {
		print_command(cmd, ">>> ");
	};

	irc.initialize();
}

int main()
{
	asio::io_service io;

	for (auto& c : connections)
	{
		c.client = std::unique_ptr<irc::Client>(new irc::Client{io});
		setup_connection(c);
	}

	io.run();
}
