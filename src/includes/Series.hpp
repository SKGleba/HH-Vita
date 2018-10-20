#ifndef SERIES
#define SERIES

#include <string>
#include <vector>
#include "Episode.hpp"

class Series {
	public:
		std::string title = "";
		std::string description = "";
		std::string link = "";
		std::string thumbnailUrl = "";
		
		std::vector<Episode> episodes;
		
		Series();
		~Series();
};


#endif