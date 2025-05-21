// File: webcam_p2p.cpp
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm> // Added for std::find
#include <cstring>   // Added for memset
#include <unistd.h>  // Added for close()
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <SDL2/SDL.h>

// Configuration
const int WIDTH = 640;
const int HEIGHT = 480;
const int PORT = 42420;
const int BROADCAST_PORT = 42421;
const int MAX_PEERS = 1;

// Global flags
std::atomic<bool> running(true);

// Debugging macros
#define DEBUG(msg) std::cout << "[DEBUG] " << msg << std::endl
#define ERROR(msg) std::cerr << "[ERROR] " << msg << std::endl

struct Peer {
    std::string ip;
    int sock;
};

int open_camera() {
    for(int i = 0; i < 4; i++) {
        std::string dev = "/dev/video" + std::to_string(i);
        int fd = open(dev.c_str(), O_RDWR);
        if(fd >= 0) {
            DEBUG("Using camera: " << dev);
            return fd;
        }
    }
    ERROR("No cameras found! Available devices:");
    system("ls /dev/video*");
    exit(1);
}

std::vector<std::string> get_local_ips() {
    std::vector<std::string> ips;
    struct ifaddrs *addrs;
    getifaddrs(&addrs);
    for(auto ifa = addrs; ifa; ifa = ifa->ifa_next) {
        if(ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            void *tmp = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char buf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmp, buf, INET_ADDRSTRLEN);
            if(strcmp(buf, "127.0.0.1") != 0)
                ips.push_back(buf);
        }
    }
    freeifaddrs(addrs);
    return ips;
}

void discover_peers(std::vector<Peer> &peers) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(BROADCAST_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    bind(sock, (sockaddr*)&addr, sizeof(addr));
    
    while(running) {
        char buf[INET_ADDRSTRLEN];
        sockaddr_in sender;
        socklen_t len = sizeof(sender);
        memset(&sender, 0, sizeof(sender));
        
        if(recvfrom(sock, buf, sizeof(buf), 0, (sockaddr*)&sender, &len) > 0) {
            std::string ip = inet_ntoa(sender.sin_addr);
            auto local_ips = get_local_ips();
            if(std::find(local_ips.begin(), local_ips.end(), ip) == local_ips.end()) {
                DEBUG("Discovered peer: " << ip);
                peers.push_back({ip, -1});
                if(peers.size() >= MAX_PEERS) break;
            }
        }
    }
    close(sock);
}

void video_loop() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("42 Webcam P2P", 
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
        WIDTH*2, HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    
    int cam = open_camera();
    v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    
    if(ioctl(cam, VIDIOC_S_FMT, &fmt) < 0) {
        ERROR("Failed to set camera format");
        v4l2_format current_fmt;
        memset(&current_fmt, 0, sizeof(current_fmt));
        ioctl(cam, VIDIOC_G_FMT, &current_fmt);
        DEBUG("Supported format: " << current_fmt.fmt.pix.pixelformat);
    }

    std::vector<Peer> peers;
    std::thread discovery(discover_peers, std::ref(peers));

    SDL_Texture* local_tex = SDL_CreateTexture(renderer, 
        SDL_PIXELFORMAT_YUY2, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    SDL_Texture* remote_tex = SDL_CreateTexture(renderer, 
        SDL_PIXELFORMAT_YUY2, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    
    unsigned char frame[WIDTH * HEIGHT * 2];
    
    while(running) {
        if(read(cam, frame, sizeof(frame)) <= 0) {
            ERROR("Failed to read camera frame");
            continue;
        }
        
        for(auto &peer : peers) {
            if(peer.sock == -1) {
                peer.sock = socket(AF_INET, SOCK_STREAM, 0);
                sockaddr_in addr;
                memset(&addr, 0, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_port = htons(PORT);
                inet_pton(AF_INET, peer.ip.c_str(), &addr.sin_addr);
                
                if(connect(peer.sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
                    ERROR("Failed to connect to " << peer.ip);
                    peer.sock = -1;
                }
            }
            
            if(peer.sock != -1) {
                send(peer.sock, frame, sizeof(frame), 0);
            }
        }
        
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) running = false;
        }
        
        SDL_UpdateTexture(local_tex, NULL, frame, WIDTH*2);
        
        SDL_RenderClear(renderer);
        SDL_Rect local_rect = {0, 0, WIDTH, HEIGHT};
        SDL_Rect remote_rect = {WIDTH, 0, WIDTH, HEIGHT};
        SDL_RenderCopy(renderer, local_tex, NULL, &local_rect);
        SDL_RenderCopy(renderer, remote_tex, NULL, &remote_rect);
        SDL_RenderPresent(renderer);
    }
    
    close(cam);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main() {
    std::cout << "Starting 42 Webcam P2P...\n";
    auto ips = get_local_ips();
    std::cout << "Local IPs: ";
    for(const auto& ip : ips) std::cout << ip << " ";
    std::cout << "\n";
    
    video_loop();
    return 0;
}
