#ifndef EPISODE
#define EPISODE

#include <string>
#include <vector>
#include "EpisodeVideo.hpp"

class Episode {
	public:
		int number = 0;
		std::string title = "";
		std::string description = "";
		std::string link = "";
		std::string thumbnailUrl = "";
		
		std::vector<EpisodeVideo> episodeVideos;
		
		Episode();
		~Episode();
};


#endif