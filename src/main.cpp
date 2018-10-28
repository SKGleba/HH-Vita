/*
	HentaiHaven-Vita by SKGleba & coderx3
	All Rights Reserved
*/
#include <psp2/kernel/processmgr.h>
#include <sstream>
#include <vector>
#include <psp2/io/fcntl.h>
#include <vita2d.h>
#include <map>
#include <psp2/appmgr.h>
#include <psp2/io/stat.h>

#include <psp2/io/stat.h>
#include <psp2/io/dirent.h>

#include <psp2/types.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/net/http.h>
#include <psp2/sysmodule.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>

#include "json.hpp"
#include "VitaPad.hpp"

class EpisodeVideo {
	public:
		std::string resolution = "";
		std::string link = "";
		
		EpisodeVideo();
		~EpisodeVideo();
};

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

VitaPad vitaPad;

std::vector<Series> allSeries;

vita2d_font * textFont;

EpisodeVideo::EpisodeVideo(){}
EpisodeVideo::~EpisodeVideo(){}
Episode::Episode(){}
Episode::~Episode(){}
Series::Series(){}
Series::~Series(){}

std::string jsonSeriesName = "series";
std::string jsonEpisodesName = "episodes";
std::string jsonEpisodeVideosName = "episodevideos";

int seriesAmount = 0;

int currentMin = 0;
int currentS = 0;
int currentE = 0;
int currentX = 0;
int currentMax = 14;

int hhonly = 0;
int ohmode = 0;
int pwdreq = 0;
int tmppwd = 0;

const char xopts[4][512] = {"Watch", "Download", "Open in browser", "Back"};
const char yopts[7][512] = {"Only list hentaihaven.org videos:", "One hand mode:", "Require password at boot:", "Delete the current password", "Update the movie database", "Save and restart the app", "Back"};
const char yenop[2][8] = {"No", "Yes"};

nlohmann::json seriesJSON;

std::string TFWrap(std::string text, int maxlen, int maxsz) {
	if (text.size() > maxlen) {
   	 for(int i = maxlen; i < text.size(); i = i + maxlen) {
       		if (text[i] == 0x20) { 
				text[i] = 0x0A;
			} else {
				text.insert(i, "\n");
			}
 	   }
	}
	if (text.size() > maxsz) {
		text[maxsz - 3] = text[maxsz - 2] = text[maxsz - 1] = 0x2E;
   	 for(int i = maxsz; i < text.size(); ++i) {
			text[i] = 0x00;
 	   }
	}
    return text;
}

std::string TFSpaceURL(std::string text) { // "Fuck me daddy" -> "Fuck%20me%20daddy"
   	 for(int i = 0; i < text.size(); i = i + 1) {
       		if (text[i] == 0x20) { 
				text[i] = 0x30;
				text.insert(i, "%2");
			}
 	   }
    return text;
}

std::string TFSpaceURL2(std::string text) { // "I want you deep inside me" -> "I_want_you_deep_inside_me"
   	 for(int i = 0; i < text.size(); i = i + 1) {
       		if (text[i] == 0x20) { 
				text[i] = 0x2D;
			}
 	   }
    return text;
}

int fcp(const char *from, const char *to) {
	long psz;
	FILE *fp = fopen(from,"rb");

	fseek(fp, 0, SEEK_END);
	psz = ftell(fp);
	rewind(fp);

	char* pbf = (char*) malloc(sizeof(char) * psz);
	fread(pbf, sizeof(char), (size_t)psz, fp);

	FILE *pFile = fopen(to, "wb");
	
	for (int i = 0; i < psz; ++i) {
			fputc(pbf[i], pFile);
	}
   
	fclose(fp);
	fclose(pFile);
	return 1;
}

int ex(const char *fname) {
    FILE *file;
    if ((file = fopen(fname, "r")))
    {
        fclose(file);
        return 1;
    }
    return 0;
}

int hasEndSlash(const char *path) {
	return path[strlen(path) - 1] == '/';
}

int removePath(const char *path) {
	SceUID dfd = sceIoDopen(path);
	if (dfd >= 0) {
		int res = 0;
		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				char *new_path = malloc(strlen(path) + strlen(dir.d_name) + 2);
				snprintf(new_path, 1024, "%s%s%s", path, hasEndSlash(path) ? "" : "/", dir.d_name);

				if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
					int ret = removePath(new_path);
					if (ret <= 0) {
						free(new_path);
						sceIoDclose(dfd);
						return ret;
					}
				} else {
					int ret = sceIoRemove(new_path);
					if (ret < 0) {
						free(new_path);
						sceIoDclose(dfd);
						return ret;
					}
				}

				free(new_path);
			}
		} while (res > 0);

		sceIoDclose(dfd);

		int ret = sceIoRmdir(path);
		if (ret < 0)
			return ret;
	} else {
		int ret = sceIoRemove(path);
		if (ret < 0)
			return ret;
	}

	return 1;
}

int loadJson(){
	int fhSeries;

	if (hhonly) {
		if (ex("ux0:data/hhapp/udb.json")) {
			fhSeries = sceIoOpen("ux0:data/hhapp/udb.json" , SCE_O_RDONLY , 0777); // user db
		} else if (ex("ux0:data/hhapp/db.json")) {
			fhSeries = sceIoOpen("ux0:data/hhapp/db.json" , SCE_O_RDONLY , 0777); // new db
		} else {
			fhSeries = sceIoOpen("app0:assets/db.json" , SCE_O_RDONLY , 0777);  // orig db
		}
	} else {
		if (ex("ux0:data/hhapp/udball.json")) {
			fhSeries = sceIoOpen("ux0:data/hhapp/udball.json" , SCE_O_RDONLY , 0777); // user db
		} else if (ex("ux0:data/hhapp/dball.json")) {
			fhSeries = sceIoOpen("ux0:data/hhapp/dball.json" , SCE_O_RDONLY , 0777); // new db
		} else {
			fhSeries = sceIoOpen("app0:assets/dball.json" , SCE_O_RDONLY , 0777);  // orig db
		}
	}
	if(fhSeries < 0){
		return -1;
	}
	int fileSize = sceIoLseek ( fhSeries, 0, SCE_SEEK_END );
	sceIoLseek ( fhSeries, 0, SCE_SEEK_SET ); 
	std::string seriesJsonString(fileSize , '\0');
	sceIoRead(fhSeries , &seriesJsonString[0] , fileSize );
	sceIoClose(fhSeries);
	
	seriesJSON = nlohmann::json::parse(seriesJsonString);
	
	return 0;
}

int verifytimg() {
static char buf[4];
 int fd = sceIoOpen("ux0:data/hhapp/tempimg.jpg" , SCE_O_RDONLY , 0777);
 sceIoRead(fd, buf, 4);
 sceIoClose(fd);
 if (buf[0] == "_") {
	return 0;
 } else {
	return 1;
 }
}

void SetupVita2D(){
	vita2d_init();
	vita2d_set_clear_color(RGBA8(0x40, 0x40, 0x40, 0xFF));
	textFont = vita2d_load_font_file("app0:assets/font.ttf");
}

int dlfile(const char *src, const char *dst, const char *tmpf) {
	int ret;
	int tpl = sceHttpCreateTemplate("hhdl template", 2, 1);
	if (tpl < 0) {
		return tpl;
	}
	int conn = sceHttpCreateConnectionWithURL(tpl, src, 0);
	if (conn < 0) {
		return conn;
	}
	int req = sceHttpCreateRequestWithURL(conn, 0, src, 0);
	if (req < 0) {
		return req;
	}
	ret = sceHttpSendRequest(req, NULL, 0);
	if (ret < 0) {
		return ret;
	}
	unsigned char buf[4096] = {0};

	long long length = 0;
	ret = sceHttpGetResponseContentLength(req, &length);

	if (ex(tmpf)) sceIoRemove(tmpf);
	int fd = sceIoOpen(tmpf, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	int total_read = 0;
	if (fd < 0) {
		return fd;
	}
	while (1) {
		int read = sceHttpReadData(req, buf, sizeof(buf));
		if (read < 0) {
			return read;
		}
		if (read == 0)
			break;
		ret = sceIoWrite(fd, buf, read);
		if (ret < 0 || ret != read) {
			if (ret < 0)
				return ret;
			return -1;
		}
		total_read += read;
	}
	ret = sceIoClose(fd);

	if (ex(dst)) sceIoRemove(dst);

	sceIoRename(tmpf, dst);

	return 0;
}

void netinit() {
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
	SceNetInitParam netInitParam;
	int size = 1*1024*1024;
	netInitParam.memory = malloc(size);
	netInitParam.size = size;
	netInitParam.flags = 0;
	sceNetInit(&netInitParam);
	sceNetCtlInit();
	sceSysmoduleLoadModule(SCE_SYSMODULE_HTTP);
	sceHttpInit(1*1024*1024);
}

void dldb() {
	vita2d_start_drawing();
	vita2d_clear_screen();
	vita2d_font_draw_text(textFont, 20, 90, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 32, "Updating the database...");
	vita2d_end_drawing();
	vita2d_swap_buffers();
		dlfile("http://217.61.5.187/psvita/hhapp/HH.json", "ux0:data/hhapp/db.json", "ux0:data/hhapp/adb.json");
		dlfile("http://217.61.5.187/psvita/hhapp/ALL.json", "ux0:data/hhapp/dball.json", "ux0:data/hhapp/adball.json");
}

void LoadSeriesSmart(){
	// need to make it smarter than this :facepalm: without clearing everything
		allSeries.clear();
		if(!seriesJSON.is_null()){
			if(!seriesJSON[jsonSeriesName].is_null()){
				seriesAmount = seriesJSON[jsonSeriesName].size();
				for(int i = currentMin; i < currentMax; i++){
					Series series;
					if(!seriesJSON[jsonSeriesName][i]["title"].is_null()){
						series.title = seriesJSON[jsonSeriesName][i]["title"].get<std::string>();
					}else{
						series.title = "";
					}
					if(!seriesJSON[jsonSeriesName][i]["description"].is_null()){
						series.description = seriesJSON[jsonSeriesName][i]["description"].get<std::string>();
					}else{
						series.description = "_";
					}
					if(!seriesJSON[jsonSeriesName][i]["link"].is_null()){
						series.link = seriesJSON[jsonSeriesName][i]["link"].get<std::string>();
					}else{
						series.link = "";
					}
					if(!seriesJSON[jsonSeriesName][i]["thumbnailUrl"].is_null()){
						series.thumbnailUrl = seriesJSON[jsonSeriesName][i]["thumbnailUrl"].get<std::string>();
					}else{
						series.thumbnailUrl = "";
					}
				
					series.episodes.clear();
					if(!seriesJSON[jsonSeriesName][i][jsonEpisodesName].is_null()){
						int episodesAmount = seriesJSON[jsonSeriesName][i][jsonEpisodesName].size();
						for(int s = 0; s < episodesAmount ; s++){
							Episode e;
							/* For some reason parsing the number crashes it 
							if(!seriesJSON[jsonSeriesName][i][jsonEpisodesName][s]["number"].is_null()){
								e.number = seriesJSON[jsonSeriesName][i][jsonEpisodesName][s]["number"].get<int>();
							}else{
								e.number = "";
							}*/
							if(!seriesJSON[jsonSeriesName][i][jsonEpisodesName][s]["title"].is_null()){
								e.title = seriesJSON[jsonSeriesName][i][jsonEpisodesName][s]["title"].get<std::string>();
							}else{
								e.title = "";
							}
							if(!seriesJSON[jsonSeriesName][i][jsonEpisodesName][s]["description"].is_null()){
								e.description = seriesJSON[jsonSeriesName][i][jsonEpisodesName][s]["description"].get<std::string>();
							}else{
								e.description = "";
							}
							if(!seriesJSON[jsonSeriesName][i][jsonEpisodesName][s]["link"].is_null()){
								e.link = seriesJSON[jsonSeriesName][i][jsonEpisodesName][s]["link"].get<std::string>();
							}else{
								e.link = "";
							}
							if(!seriesJSON[jsonSeriesName][i][jsonEpisodesName][s]["thumbnailUrl"].is_null()){
								e.thumbnailUrl = seriesJSON[jsonSeriesName][i][jsonEpisodesName][s]["thumbnailUrl"].get<std::string>();
							}else{
								e.thumbnailUrl = "";
							}
							
							if(!seriesJSON[jsonSeriesName][i][jsonEpisodesName][s][jsonEpisodeVideosName].is_null()){
								int episodeVideosAmount = seriesJSON[jsonSeriesName][i][jsonEpisodesName][s][jsonEpisodeVideosName].size();
								e.episodeVideos.clear();
								for(int ev = 0; ev < episodeVideosAmount; ev++){
									EpisodeVideo epVid;
									if(!seriesJSON[jsonSeriesName][i][jsonEpisodesName][s][jsonEpisodeVideosName][ev]["resolution"].is_null()){
										epVid.resolution = seriesJSON[jsonSeriesName][i][jsonEpisodesName][s][jsonEpisodeVideosName][ev]["resolution"].get<std::string>();
									}else{
										epVid.resolution = "";
									}
									if(!seriesJSON[jsonSeriesName][i][jsonEpisodesName][s][jsonEpisodeVideosName][ev]["link"].is_null()){
										epVid.link = seriesJSON[jsonSeriesName][i][jsonEpisodesName][s][jsonEpisodeVideosName][ev]["link"].get<std::string>();
									}else{
										epVid.link = "";
									}
									
									e.episodeVideos.push_back(epVid);
								}
							}
							series.episodes.push_back(e);
						}
					}
					
					allSeries.push_back(series);
				}
			}
		}
}


int rape(int S, int E, int cX) {
char uri[512];
std::string yaxtsf = allSeries[S].title.c_str();
std::string yaxtstf = allSeries[S].title.c_str();
std::string rapevarr = allSeries[S].episodes[E].title.c_str();
yaxtsf = TFSpaceURL(yaxtsf);
yaxtstf = TFSpaceURL2(yaxtstf);
rapevarr[0] = 0x2D;
 if (cX == 0) {
	sprintf(uri, "videostreaming:play?url='%s%s %d%s%s.mp4'", allSeries[S].link.c_str(), allSeries[S].title.c_str(), (E + 1), "%2", allSeries[S].episodes[E].title.c_str());
 } else if (cX == 1) {
	sprintf(uri, "webmodal: %s%s%s%d%s%s.mp4", allSeries[S].link.c_str(), yaxtsf.c_str(), "%20", (E + 1), "%2", allSeries[S].episodes[E].title.c_str());
 } else if (cX == 2) {
	sprintf(uri, "webmodal: http://hentaimama.com/tvshows/%s/", yaxtstf.c_str()); //Should exist
 } else {
	return 0;
 }
sceAppMgrLaunchAppByUri(0x20000, uri);
return 0;
}

int chknorape(const char *rapevar) {
 if (rapevar[0] == 0x30) {
 	return 1;
 } else {
 	return 0;
 }
}

int fap(int S, int E, int cX) {

if (chknorape(allSeries[S].episodes[E].title.c_str()) == 1) {
	rape(S, E, cX);
	return 0;
}

char uri[512];

if (allSeries[S].episodes[E].episodeVideos.size() == 1 && cX < 2) {
	if (cX == 0) {
		sprintf(uri, "videostreaming:play?url='%s'", allSeries[S].episodes[E].episodeVideos[0].link.c_str());
	} else if (cX == 1) {
		sprintf(uri, "webmodal: %s", allSeries[S].episodes[E].episodeVideos[0].link.c_str());
	}
	sceAppMgrLaunchAppByUri(0x20000, uri);
	return 0;
}

 if (cX < 2) {
	int xxxloop = 1;
	int X = 0;
	while(xxxloop == 1) {
	vita2d_start_drawing();
	vita2d_clear_screen();
		sceKernelDelayThread(0.15 * 1000 * 1000);
		vita2d_font_draw_text(textFont, 20, 60, RGBA8(0, 0xFF, 0xFF, 0xFF), 32, "Title:");
		vita2d_font_draw_textf(textFont, 104, 60, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 32, "%s / Ep%d", allSeries[S].title.c_str(), (E+1));
		vita2d_font_draw_text(textFont, 20, 150, RGBA8(0, 0xFF, 0xFF, 0xFF), 32, "Quality:");
		for(int i = 0; i < allSeries[S].episodes[E].episodeVideos.size(); i++){
			if (i == X) {
				vita2d_font_draw_textf(textFont, 220, 220 + i*32, RGBA8(0xFF, 0x00, 0xFF, 0xFF), 32, "%s", allSeries[S].episodes[E].episodeVideos[i].resolution.c_str());
			} else {
				vita2d_font_draw_textf(textFont, 220, 220 + i*32, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 32, "%s", allSeries[S].episodes[E].episodeVideos[i].resolution.c_str());
			}
		}
	vita2d_end_drawing();
	vita2d_swap_buffers();
		vitaPad.Read();
		while(!vitaPad.circle && !vitaPad.cross && !vitaPad.up && !vitaPad.down && !vitaPad.start && !vitaPad.lefttrigger){
			vitaPad.Read();
			sceKernelDelayThread(100);
		}
		if (vitaPad.lefttrigger) {
			if (ohmode) xxxloop = 0;
		} else if (vitaPad.circle) {
			xxxloop = 0;
		} else if (vitaPad.start) {
			if (ohmode == 1) {
				sceKernelDelayThread(0.14 * 1000 * 1000);
				sprintf(uri, "videostreaming:play?url='%s'", allSeries[S].episodes[E].episodeVideos[X].link.c_str());
				sceAppMgrLaunchAppByUri(0x20000, uri);
				xxxloop = 0;
			}
		} else if (vitaPad.cross) {
			if (cX == 0) {
				sceKernelDelayThread(0.14 * 1000 * 1000);
				sprintf(uri, "videostreaming:play?url='%s'", allSeries[S].episodes[E].episodeVideos[X].link.c_str());
				sceAppMgrLaunchAppByUri(0x20000, uri);
				xxxloop = 0;
			} else if (cX == 1) {
				sceKernelDelayThread(0.14 * 1000 * 1000);
				sprintf(uri, "webmodal: %s", allSeries[S].episodes[E].episodeVideos[X].link.c_str());
				sceAppMgrLaunchAppByUri(0x20000, uri);
				xxxloop = 0;
			}
		} else if (vitaPad.up) {
			if (X > 0) X-=1;
		} else if (vitaPad.down) {
			if (X < (allSeries[S].episodes[E].episodeVideos.size() - 1)) X+=1;
		}
	}
 } else if (cX == 2) {
		sprintf(uri, "webmodal: %s", allSeries[S].episodes[E].link.c_str());
		sceAppMgrLaunchAppByUri(0x20000, uri);
 } else {
		return 0;
 }
}

int fapprep() {
vita2d_texture *thumbn;
vita2d_start_drawing();
vita2d_clear_screen();
vita2d_font_draw_text(textFont, 100, 125, RGBA8(0xFF, 0, 0xFF, 0xFF), 32, "Loading info...");
vita2d_end_drawing();
vita2d_swap_buffers();
dlfile(allSeries[currentS].thumbnailUrl.c_str(), "ux0:data/hhapp/tempimg.jpg", "ux0:data/hhapp/atempimg.jpg");
static char buf[4];
int fd = sceIoOpen("ux0:data/hhapp/tempimg.jpg" , SCE_O_RDONLY , 0777);
sceIoRead(fd, buf, 4);
 if (buf[0] == 0xFF) {
	thumbn = vita2d_load_JPEG_file("ux0:data/hhapp/tempimg.jpg");
 } else {
	thumbn = vita2d_load_PNG_file("ux0:data/hhapp/tempimg.jpg");
 }
sceIoClose(fd);
std::string yaxtnf = allSeries[currentS].description.c_str();
std::string yaxttf = allSeries[currentS].title.c_str();
yaxttf = TFWrap(yaxttf, 48, 96);
yaxtnf = TFWrap(yaxtnf, 64, 768);
vita2d_start_drawing();
vita2d_clear_screen();
vita2d_font_draw_textf(textFont, 3, 30, RGBA8(0xFF, 0, 0xFF, 0xFF), 32, "%s", yaxttf.c_str());
vita2d_font_draw_text(textFont, 10, 125, RGBA8(0, 0xFF, 0xFF, 0xFF), 32, "Options:");
vita2d_font_draw_text(textFont, 200, 110, RGBA8(0, 0xFF, 0xFF, 0xFF), 32, "Press START to watch");
vita2d_font_draw_text(textFont, 200, 140, RGBA8(0, 0xFF, 0xFF, 0xFF), 32, "Press CIRCLE to go back");
vita2d_font_draw_textf(textFont, 3, 180, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 32, "%s", yaxtnf.c_str());
vita2d_draw_texture(thumbn , 780 , 3);
vita2d_end_drawing();
vita2d_swap_buffers();
 while(!vitaPad.circle && !vitaPad.start && !vitaPad.lefttrigger) {
	vitaPad.Read();
	sceKernelDelayThread(100);
 }
 if (vitaPad.start) {
 	currentE = 0;
 	fap(currentS, currentE, currentX);
 }
vita2d_free_texture(thumbn);
}

int dmore() {
int faploop = 1;
currentE = 0;
 while(faploop == 1) {
	sceKernelDelayThread(0.15 * 1000 * 1000);
	if (allSeries[currentS].description.length() > 1) {
		std::string yaxt = allSeries[currentS].description.c_str();
    	yaxt = TFWrap(yaxt, 64, 512);
		vita2d_start_drawing();
		vita2d_clear_screen();
		vita2d_font_draw_text(textFont, 20, 60, RGBA8(0, 0xFF, 0xFF, 0xFF), 32, "Title:");
		vita2d_font_draw_textf(textFont, 104, 60, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 32, "%s", allSeries[currentS].title.c_str());
		vita2d_font_draw_text(textFont, 20, 100, RGBA8(0, 0xFF, 0xFF, 0xFF), 32, "Description:");
		vita2d_font_draw_textf(textFont, 3, 132, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 32, "%s", yaxt.c_str());
		vita2d_font_draw_text(textFont, 20, 400, RGBA8(0, 0xFF, 0xFF, 0xFF), 32, "Episode:");
		vita2d_font_draw_textf(textFont, 150, 400, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 32, "%d/%d", (currentE + 1), allSeries[currentS].episodes.size());
	} else {
		std::string yaxt = allSeries[currentS].episodes[currentE].title.c_str();
    	yaxt = TFWrap(yaxt, 64, 256);
		vita2d_start_drawing();
		vita2d_clear_screen();
		vita2d_font_draw_text(textFont, 20, 60, RGBA8(0, 0xFF, 0xFF, 0xFF), 32, "Title:");
		vita2d_font_draw_textf(textFont, 104, 60, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 32, "%s", allSeries[currentS].title.c_str());
		vita2d_font_draw_text(textFont, 20, 100, RGBA8(0, 0xFF, 0xFF, 0xFF), 32, "Description:");
		vita2d_font_draw_text(textFont, 3, 132, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 32, "N/A");
		vita2d_font_draw_text(textFont, 20, 170, RGBA8(0, 0xFF, 0xFF, 0xFF), 32, "Episode title:");
		vita2d_font_draw_textf(textFont, 3, 202, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 32, "%s", yaxt.c_str());
		vita2d_font_draw_text(textFont, 20, 400, RGBA8(0, 0xFF, 0xFF, 0xFF), 32, "Episode:");
		vita2d_font_draw_textf(textFont, 150, 400, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 32, "%d/%d", (currentE + 1), allSeries[currentS].episodes.size());
	}
		for(int i = 0; i < 4; i++){
			if (i == currentX) {
				vita2d_font_draw_textf(textFont, 250, 420 + i*32, RGBA8(0xFF, 0x00, 0xFF, 0xFF), 32, "%s", xopts[i]);
			} else {
				vita2d_font_draw_textf(textFont, 250, 420 + i*32, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 32, "%s", xopts[i]);
			}
		}
	vita2d_end_drawing();
	vita2d_swap_buffers();
	vitaPad.Read();
		while(!vitaPad.circle && !vitaPad.cross && !vitaPad.righttrigger && !vitaPad.up && !vitaPad.down && !vitaPad.lefttrigger && !vitaPad.left && !vitaPad.right && !vitaPad.select && !vitaPad.start){
		vitaPad.Read();
		sceKernelDelayThread(100);
	}
	if (vitaPad.select) {
		fapprep();
	} else if (vitaPad.start) {
		if (ohmode == 1) fap(currentS, currentE, 0);
	} else if (vitaPad.circle) {
		faploop = 0;
	} else if (vitaPad.cross) {
		sceKernelDelayThread(0.14 * 1000 * 1000);
		if (currentX < 3) {
			fap(currentS, currentE, currentX);
		} else {
			faploop = 0;
		}
	} else if (vitaPad.righttrigger) {
		if(currentMax >= seriesAmount) {
			if (currentS < 13) currentS += 1;
		} else {
			currentS = 0;
			currentMin += 1;
			currentMax += 1;
		}
		currentE = 0;
		LoadSeriesSmart();
	} else if (vitaPad.lefttrigger) {
		if (!ohmode) {
			if(currentMax >= seriesAmount) {
				if (currentS > 0) {
					currentS -= 1;
				} else {
					currentMin -= 1;
					currentMax -= 1;
				}
			} else if (currentMin > 0) {
				currentS = 0;
				currentMin -= 1;
				currentMax -= 1;
			}
			currentE = 0;
			LoadSeriesSmart();
		} else {
			faploop = 0;
		}
	} else if (vitaPad.right) {
		if ((currentE + 1) < allSeries[currentS].episodes.size()) {
			currentE+=1;
		} else if ((currentE + 1) >= allSeries[currentS].episodes.size() && ohmode == 1) {
			if(currentMax >= seriesAmount) {
				if (currentS < 13) currentS += 1;
			} else {
				currentS = 0;
				currentMin += 1;
				currentMax += 1;
			}
			currentE = 0;
			LoadSeriesSmart();
		}
	} else if (vitaPad.left) {
		if (currentE > 0) {
			currentE-=1;
		} else if (currentE <= 0 && ohmode == 1) {
			if(currentMax >= seriesAmount) {
				if (currentS > 0) {
					currentS -= 1;
				} else {
					currentMin -= 1;
					currentMax -= 1;
				}
			} else if (currentMin > 0) {
				currentS = 0;
				currentMin -= 1;
				currentMax -= 1;
			}
			currentE = 0;
			LoadSeriesSmart();
		}
	} else if (vitaPad.up) {
		if (currentX > 0) currentX-=1;
	} else if (vitaPad.down) {
		if (currentX < 3) currentX+=1;
	}

 }
}

void settings() {
int sloop = 1;
int cY = 0;
 static char buf[4];
 int fd = sceIoOpen("ux0:data/hhapp/init.hh" , SCE_O_RDONLY , 0777);
 sceIoRead(fd, buf, 4);
 sceIoClose(fd);
 while(sloop == 1) {
	sceKernelDelayThread(0.15 * 1000 * 1000);
		vita2d_start_drawing();
		vita2d_clear_screen();
		vita2d_font_draw_text(textFont, 100, 60, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 32, "HentaiHaven - Vita v3.0 by SKGleba & coderx3");
		vita2d_font_draw_text(textFont, 3, 520, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 32, "twitter.com/skgleba");
		for(int i = 0; i < 7; i++){
			if (i == cY && i < 3) {
				vita2d_font_draw_textf(textFont, 200, 120 + i*32, RGBA8(0xFF, 0x00, 0xFF, 0xFF), 32, "%s %s", yopts[i], yenop[(buf[i] - 0x30)]);
			} else if (i < 3) {
				vita2d_font_draw_textf(textFont, 200, 120 + i*32, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 32, "%s %s", yopts[i], yenop[(buf[i] - 0x30)]);
			} else if (i == cY && i > 2) {
				vita2d_font_draw_textf(textFont, 200, 120 + i*32, RGBA8(0xFF, 0x00, 0xFF, 0xFF), 32, "%s", yopts[i]);
			} else if (i > 2) {
				vita2d_font_draw_textf(textFont, 200, 120 + i*32, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 32, "%s", yopts[i]);
			}
		}
	vita2d_end_drawing();
	vita2d_swap_buffers();
	vitaPad.Read();
		while(!vitaPad.circle && !vitaPad.cross && !vitaPad.up && !vitaPad.down){
		vitaPad.Read();
		sceKernelDelayThread(100);
	}
	if (vitaPad.circle) {
		sloop = 0;
	} else if (vitaPad.cross) {
		sceKernelDelayThread(0.14 * 1000 * 1000);
		if (cY < 3) {
			if (buf[cY] == 0x31) {
				buf[cY] = 0x30;
			} else {
				buf[cY] = 0x31;
			}
		} else if (cY == 3) {
			removePath("ux0:data/hhapp/pwd");
		} else if (cY == 4) {
			dldb();
			sceAppMgrLoadExec("app0:eboot.bin", NULL, NULL);
		} else if (cY == 5) {
			fd = sceIoOpen("ux0:data/hhapp/init.hh" , SCE_O_WRONLY , 0777);
			sceIoWrite(fd, buf, 4);
			sceIoClose(fd);
			sceAppMgrLoadExec("app0:eboot.bin", NULL, NULL);
		} else if (cY == 6) {
			sloop = 0;
		}	
	} else if (vitaPad.up) {
		if (cY > 0) cY-=1;
	} else if (vitaPad.down) {
		if (cY < 6) cY+=1;
	}

 }
}

int main(int argc, char *argv[]) {

if (!ex("ux0:data/hhapp/init.hh")) {
	sceIoMkdir("ux0:data/hhapp/", 0777);
	fcp("app0:defset.x", "ux0:data/hhapp/init.hh");
	fcp("app0:assets/db.json", "ux0:data/hhapp/db.json");
	fcp("app0:assets/dball.json", "ux0:data/hhapp/dball.json");
}

 static char buf[4];
 int fd = sceIoOpen("ux0:data/hhapp/init.hh" , SCE_O_RDONLY , 0777);
 sceIoRead(fd, buf, 4);
 sceIoClose(fd);
 hhonly = (buf[0] - 0x30);
 ohmode = (buf[1] - 0x30);
 pwdreq = (buf[2] - 0x30);
 tmppwd = (buf[3] - 0x30);

	if (pwdreq == 1 && tmppwd == 0) {
		sceAppMgrLoadExec("app0:mp4p.bin", NULL, NULL);
	} else if (pwdreq == 1 && tmppwd == 1) {
		fd = sceIoOpen("ux0:data/hhapp/init.hh" , SCE_O_WRONLY , 0777);
		buf[3] = 0x30;
		sceIoWrite(fd, buf, 4);
		sceIoClose(fd);
	}

	netinit();

	if(loadJson() != 0){
		return 0;
	}

	SetupVita2D();
	
	LoadSeriesSmart();
	
		vita2d_start_drawing();
		vita2d_clear_screen();
		vita2d_font_draw_text(textFont, 200, 60, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 32, "HentaiHaven - Vita v3.0");

		for(int i = 0; i < allSeries.size(); i++){
			if (i == currentS) {
				vita2d_font_draw_textf(textFont, 20, 120 + i*32, RGBA8(0xFF, 0x00, 0xFF, 0xFF), 32, "%s", allSeries[i].title.c_str());
			} else {
				vita2d_font_draw_textf(textFont, 20, 120 + i*32, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 32, "%s", allSeries[i].title.c_str());
			}
			
		}
		vita2d_end_drawing();
		vita2d_swap_buffers();

	while(true){
		vitaPad.Read();
		while(!vitaPad.cross && !vitaPad.righttrigger && !vitaPad.up && !vitaPad.down && !vitaPad.lefttrigger && !vitaPad.start && !vitaPad.select && !vitaPad.right){
			vitaPad.Read();
			sceKernelDelayThread(100);
		}
		if (vitaPad.cross) {
			sceKernelDelayThread(0.14 * 1000 * 1000);
			dmore();
		} else if (vitaPad.start) {
			sceAppMgrLoadExec("app0:mp4p.bin", NULL, NULL);
		} else if (vitaPad.right) {
			sceKernelDelayThread(0.14 * 1000 * 1000);
			if (ohmode) dmore();
		} else if (vitaPad.select) {
			settings();
		} else if (vitaPad.righttrigger) {
			currentMin += 14;
			currentMax += 14;
		} else if (vitaPad.lefttrigger) {
			currentMin -= 14;
			currentMax -= 14;
		} else if (vitaPad.up) {
			if(currentMax >= seriesAmount) {
				if (currentS > 0) {
					currentS -= 1;
				} else {
					currentMin -= 1;
					currentMax -= 1;
				}
			} else {
				currentS = 0;
				currentMin -= 1;
				currentMax -= 1;
			}
		} else if (vitaPad.down) {
			if(currentMax >= seriesAmount) {
				if (currentS < 13) currentS += 1;
			} else {
				currentS = 0;
				currentMin += 1;
				currentMax += 1;
			}
		}
		if(currentMax >= seriesAmount){
			currentMin = seriesAmount - 14;
			currentMax = seriesAmount;
		}
		if(currentMin >= currentMax){
			currentMin = currentMax - 14;
		}
		if(currentMin < 0 || currentMax < 0){
			currentMin = 0;
			currentMax = 14;
		}
		
		LoadSeriesSmart();
		
		vita2d_start_drawing();
		vita2d_clear_screen();
		
		vita2d_font_draw_text(textFont, 200, 60, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 32, "HentaiHaven - Vita v3.0");

		for(int i = 0; i < allSeries.size(); i++){
			if (i == currentS) {
				vita2d_font_draw_textf(textFont, 20, 120 + i*32, RGBA8(0xFF, 0x00, 0xFF, 0xFF), 32, "%s", allSeries[i].title.c_str());
			} else {
				vita2d_font_draw_textf(textFont, 20, 120 + i*32, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 32, "%s", allSeries[i].title.c_str());
			}
			
		}

		vita2d_end_drawing();
		vita2d_swap_buffers();
		sceKernelDelayThread(0.15 * 1000 * 1000);
	}	
	
	sceKernelExitProcess(0);

    return 0;
}
