/*
 * Anope IRC Services
 *
 * Copyright (C) 2010-2016 Anope Team <team@anope.org>
 *
 * This file is part of Anope. Anope is free software; you can
 * redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software
 * Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see see <http://www.gnu.org/licenses/>.
 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

static std::string get_git_hash(const std::string &git_dir)
{
	std::fstream fd;
	std::string filebuf;

	fd.open((git_dir + "/HEAD").c_str(), std::ios::in);
	if (!fd.is_open())
		return "";
	if (!getline(fd, filebuf) || filebuf.find("ref: ") != 0)
	{
		fd.close();
		return "";
	}

	fd.close();

	filebuf = filebuf.substr(5);
	fd.open((git_dir + "/" + filebuf).c_str(), std::ios::in);
	if (!fd.is_open())
		return "";
	if (!getline(fd, filebuf))
	{
		fd.close();
		return "";
	}
	fd.close();

	return "g" + filebuf.substr(0, 7);
}

static bool read_version_sh(const std::string &version_sh, std::map<std::string, std::string> &versions)
{
	std::fstream fd(version_sh.c_str(), std::ios::in);
	if (!fd.is_open())
	{
		std::cerr << "Error: Unable to open src/version.sh for reading: " << version_sh << std::endl;
		return false;
	}

	std::string filebuf;
	while (getline(fd, filebuf))
	{
		if (!filebuf.find("VERSION_"))
		{
			size_t eq = filebuf.find('=');

			std::string type = filebuf.substr(0, eq);
			std::string value = filebuf.substr(eq + 1);

			versions[type] = value;
		}
	}

	fd.close();

	return true;
}

static bool write_build_h(const std::string &buildh, const std::string &git_version)
{
	std::fstream fd(buildh.c_str(), std::ios::in);

	std::string existing_version;
	if (fd.is_open())
	{
		for (std::string filebuf; getline(fd, filebuf);)
		{
			if (!filebuf.find("#define VERSION_GIT"))
			{
				std::size_t q = filebuf.find('"');
				if (q == std::string::npos)
					break;

				std::size_t q2 = filebuf.find('"', q + 1);
				if (q2 == std::string::npos)
					break;

				existing_version = filebuf.substr(q + 1, q2 - q - 1);
			}
		}

		fd.close();
	}

	if (existing_version == git_version)
		return true;

	fd.clear();
	fd.open(buildh.c_str(), std::ios::out);
	if (!fd.is_open())
	{
		std::cerr << "Error: Unable to open build.h for writing: " << buildh << std::endl;
		return false;
	}

	fd << "/* This file is automatically generated by version.cpp - do not edit it! */" << std::endl;
	if (!git_version.empty())
		fd << "#define VERSION_GIT \"" << git_version << "\"" << std::endl;
	fd.close();

	return true;
}

static void read_version_h(const std::string &versionh, std::map<std::string, std::string> &versions)
{
	std::fstream fd(versionh.c_str(), std::ios::in);

	if (!fd.is_open())
		return;

	for (std::string filebuf; getline(fd, filebuf);)
	{
		if (!filebuf.find("#define VERSION_"))
		{
			size_t space = filebuf.substr(8).find(' ');

			std::string name = filebuf.substr(8).substr(0, space),
				version = filebuf.substr(8).substr(space + 1);

			versions[name] = version;
		}
	}

	fd.close();
}

static bool write_version_h(const std::string &versionh, const std::map<std::string, std::string> &versions)
{
	std::fstream fd(versionh.c_str(), std::ios::out);

	if (!fd.is_open())
		return false;

	for (std::map<std::string, std::string>::const_iterator it = versions.begin(); it != versions.end(); ++it)
	{
		fd << "#define " << it->first << " " << it->second << std::endl;
	}

	fd.close();

	return true;
}

int main(int argc, char *argv[])
{
	if (argc < 4)
	{
		std::cerr << "Syntax: " << argv[0] << " <base> <version.h> <build.h>" << std::endl;
		return 1;
	}

	std::string version_sh = std::string(argv[1]) + "/src/version.sh";
	std::string git_dir = std::string(argv[1]) + "/.git";
	std::string versionh = argv[2];
	std::string buildh = argv[3];

	std::map<std::string, std::string> versions, old_versions;

	if (!read_version_sh(version_sh, versions))
		return -1;

	std::string git_version = get_git_hash(git_dir);
	if (!write_build_h(buildh, git_version))
		return -1;

	read_version_h(versionh, old_versions);

	if (versions == old_versions)
		return 0;

	if (!write_version_h(versionh, versions))
		return -1;

	return 0;
}
