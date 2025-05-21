#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <SDL2/SDL.h>

#define WIDTH 640
#define HEIGHT 480

int main(int argc, char** argv) {
    if(argc != 2) {
        printf("Kullanım: %s <server_ip>\n", argv[0]);
        exit(1);
    }

    // SDL başlatma
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Webcam", SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture* texture = SDL_CreateTexture(renderer, 
        SDL_PIXELFORMAT_YUY2, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    // Soket bağlantısı
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, argv[1], &addr.sin_addr);
    
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));

    // Görüntü alımı ve gösterim
    unsigned char buffer[WIDTH * HEIGHT * 2];
    while(1) {
        recv(sock, buffer, sizeof(buffer), 0);
        SDL_UpdateTexture(texture, NULL, buffer, WIDTH*2);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
