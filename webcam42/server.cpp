#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/videodev2.h>
#include <SDL2/SDL.h>

#define WIDTH 640
#define HEIGHT 480
#define PORT 8080

int main() {
    // Video4Linux2 ayarları
    int fd = open("/dev/video0", O_RDWR);
    if(fd < 0) { perror("Video açılamadı"); exit(1); }

    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    if(ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) { perror("Format ayarlanamadı"); exit(1); }

    // Soket oluşturma
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(sock, 1);
    int client = accept(sock, NULL, NULL);

    // Görüntü aktarımı
    unsigned char buffer[WIDTH * HEIGHT * 2];
    while(1) {
        read(fd, buffer, sizeof(buffer));
        send(client, buffer, sizeof(buffer), 0);
    }

    close(fd);
    close(sock);
    return 0;
}
