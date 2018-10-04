#include "irc.hpp"

int main()
{
	irc::Client irc;

	irc.server = "irc.mibbit.net";

	irc.on_auth = [&](const irc::Command&) {
		irc.command({"join", "#bots"});
	};

	auto print_command = [&](const irc::Command& cmd, const std::string_view direction_string) {
		if (!cmd.prefix.empty())
		{
			fmt::print(fmt::color::gray, "{} ", cmd.prefix);
		}

		fmt::print(fmt::color::gray, "{}", direction_string);
		fmt::print(fmt::color::orange, "{} ", cmd.name);
		fmt::print("{}\n", fmt::join(cmd.parameters, " "));
	};

	irc.on_recv = [&](const irc::Command& cmd) {
		print_command(cmd, "<<< ");

		if (cmd.name == "privmsg")
		{
			if (cmd.parameters.size() != 2)
			{
				return;
			}

			if (cmd.parameters[1] == "quit")
			{
				irc.command({"quit"});
			}
		}
	};

	irc.on_send = [&](const irc::Command& cmd) {
		print_command(cmd, ">>> ");
	};

	irc.start();
}
