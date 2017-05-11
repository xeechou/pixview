#include <vector>
#include <string>
#include <magic.h>
#include <boost/filesystem.hpp>
#include <iostream>


namespace fs=boost::filesystem;

static int iterator_through_dir(const char *p, const char *thefile, magic_t& magic)
{
	std::vector<std::string> imgs;
	const char *mime_name;
	fs::path path(p);
	std::string img(thefile);
	
	int count = 0;
	for (fs::directory_iterator itr(path);
	     itr != fs::directory_iterator();
	     ++itr) {
		imgs.push_back(itr->path().string());
//		mime_name = magic_file(magic, itr->path().c_str());
//		std::cout << "file " << itr->path() << "with type " << mime_name << std::endl;
		count++;
	}

	std::sort(imgs.begin(), imgs.end());
	return std::distance(imgs.begin(), std::find(imgs.begin(), imgs.end(), img));
}

int main(int argc, char *argv[])
{
	const char *mime_name;
	magic_t magic;

	magic = magic_open(MAGIC_MIME_TYPE);
	magic_load(magic, NULL);
	magic_compile(magic, NULL);
	std::cout << iterator_through_dir(argv[1], argv[2], magic) << std::endl;
	magic_close(magic);
	return 0;
}
