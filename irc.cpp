#include "irc.hpp"

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>
#include <unordered_map>

using namespace std::string_literals;

BOOST_FUSION_ADAPT_STRUCT(
	irc::Command,
	prefix,
	name,
	parameters
)

namespace irc
{
Command::Command(std::string p_name, std::vector<std::string> p_parameters) :
	name{p_name},
	parameters{p_parameters}
{}

Command::Command(std::string p_name, std::string p_single_parameter) :
	name{p_name},
	parameters{p_single_parameter}
{}

void Client::setup_command_dispatcher()
{
	asio::async_read_until(socket, buf, '\n', [this](auto error, size_t) {
		using namespace boost::spirit::x3;

		if (!error)
		{
			std::string str;
			std::getline(std::istream{&buf}, str);

			if (str.back() == '\r')
			{
				str.resize(str.size() - 1);
			}

			auto word = +(char_ - ascii::space);

			auto prefix = lexeme[':' > word];
			auto command = lexeme[word];
			auto param = lexeme[word];
			auto long_param = lexeme[':' > +char_];

			Command cmd;

			bool r = phrase_parse(
				str.begin(),
				str.end(),
				-prefix >> command >> *(long_param | param),
				ascii::space,
				cmd
			);

			for (char& c : cmd.name)
			{
				c = ::tolower(c);
			}

			if (r)
			{
				on_recv(cmd);
				internal_command_dispatch(cmd);
			}
			else
			{
				fmt::print(fmt::color::red, "Failed parsing of command '{}'!\n", str);
			}

			// Handle a response again only if the read hasn't failed, otherwise we are getting disconnected
			setup_command_dispatcher();
		}
	});
}

void Client::internal_command_dispatch(const Command& cmd)
{
	static const std::unordered_map<std::string_view, std::function<void(const Command&)>> base_handlers {{
		{"ping", [this](const Command& cmd) {
			command({"pong", cmd.parameters});
		}},

		// TODO: find the most standard way as per RFC
		{"001", on_auth}
	}};

	if (auto it = base_handlers.find(cmd.name); it != base_handlers.end())
	{
		it->second(cmd);
	}
}

void Client::command(Command cmd)
{
	std::string composed;

	if (!cmd.prefix.empty())
	{
		composed = ':' + cmd.prefix + ' ';
	}

	composed += cmd.name;

	for (auto it = cmd.parameters.begin(); it != cmd.parameters.end(); ++it)
	{
		composed += ' ';

		if (it->find(' ') != std::string::npos)
		{
			// If we aren't dealing with the last parameter, then there's something dubious - can't have spaces within that param.
			if (it != cmd.parameters.end() - 1)
			{
				fmt::print(fmt::color::yellow, "Non-last parameter containing a space character will appear splitted: '{}'", *it);
			}
			else
			{
				composed += ':';
			}
		}

		composed += *it;
	}

	composed += '\n';

	on_send(cmd);

	asio::write(socket, asio::buffer(composed));
}

void Client::start()
{
	fmt::print("Connecting to {}:{}\n", server, port);
	asio::connect(socket, resolver.resolve(server, port));

	// Prepare for responses
	setup_command_dispatcher();

	// Send auth commands
	command({"nick", nickname});
	command({"user", {username, username, username, realname}});

	io.run();
}
}
