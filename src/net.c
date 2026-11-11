// Portable UDP (see net.h). IPv4 only: enough for direct connections, LANs
// and VPNs such as Tailscale or ZeroTier.
#include "net.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET sock_t;
#define BAD_SOCK INVALID_SOCKET
#define close_sock closesocket
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int sock_t;
#define BAD_SOCK (-1)
#define close_sock close
#endif

static sock_t sock = BAD_SOCK;

double net_now_ms(void)
{
    static uint64_t t0;
    uint64_t now = SDL_GetPerformanceCounter();
    if (!t0) t0 = now;
    return (double)(now - t0) * 1000.0 / (double)SDL_GetPerformanceFrequency();
}

bool net_open(int port)
{
#ifdef _WIN32
    static bool wsa;
    if (!wsa) { WSADATA w; if (WSAStartup(MAKEWORD(2, 2), &w)) return false; wsa = true; }
#endif
    net_close();
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == BAD_SOCK) return false;
    struct sockaddr_in a = { 0 };
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    a.sin_port = htons((uint16_t)port);
    if (bind(sock, (struct sockaddr *)&a, sizeof a) != 0) {
        fprintf(stderr, "net: cannot use UDP port %d\n", port);
        net_close();
        return false;
    }
#ifdef _WIN32
    u_long nb = 1;
    ioctlsocket(sock, FIONBIO, &nb);
#else
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
#endif
    return true;
}

void net_close(void)
{
    if (sock != BAD_SOCK) close_sock(sock);
    sock = BAD_SOCK;
}

bool net_resolve(const char *host, int port, net_addr_t *out)
{
    struct addrinfo hints = { 0 }, *res = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(host, NULL, &hints, &res) != 0 || !res) return false;
    struct sockaddr_in *sa = (struct sockaddr_in *)res->ai_addr;
    memcpy(out->ip, &sa->sin_addr, 4);
    out->port = (uint16_t)port;
    freeaddrinfo(res);
    return true;
}

bool net_addr_eq(const net_addr_t *a, const net_addr_t *b)
{
    return !memcmp(a->ip, b->ip, 4) && a->port == b->port;
}

const char *net_addr_str(const net_addr_t *a)
{
    static char buf[32];
    snprintf(buf, sizeof buf, "%u.%u.%u.%u:%u", a->ip[0], a->ip[1], a->ip[2], a->ip[3], a->port);
    return buf;
}

static void raw_send(const net_addr_t *to, const void *data, int len)
{
    if (sock == BAD_SOCK) return;
    struct sockaddr_in a = { 0 };
    a.sin_family = AF_INET;
    memcpy(&a.sin_addr, to->ip, 4);
    a.sin_port = htons(to->port);
    sendto(sock, (const char *)data, len, 0, (struct sockaddr *)&a, sizeof a);
}

/* ---------------------------------------------------------------- */
/* Simulated network conditions                                     */
/* ---------------------------------------------------------------- */

static int sim_latency, sim_jitter, sim_loss;
#define SIM_QUEUE 1024
static struct { double at; net_addr_t to; int len; uint8_t data[1400]; } simq[SIM_QUEUE];
static int simq_n;

void net_simulate(int latency_ms, int jitter_ms, int loss_pct)
{
    sim_latency = latency_ms; sim_jitter = jitter_ms; sim_loss = loss_pct;
}

static void sim_flush(void)
{
    double now = net_now_ms();
    for (int i = 0; i < simq_n;) {
        if (simq[i].at <= now) {
            raw_send(&simq[i].to, simq[i].data, simq[i].len);
            simq[i] = simq[--simq_n];
        } else i++;
    }
}

void net_send(const net_addr_t *to, const void *data, int len)
{
    if (!sim_latency && !sim_jitter && !sim_loss) { raw_send(to, data, len); return; }
    if (rand() % 100 < sim_loss) return;
    if (simq_n == SIM_QUEUE || len > (int)sizeof simq[0].data) return;
    double j = sim_jitter ? (rand() % (2 * sim_jitter + 1)) - sim_jitter : 0;
    simq[simq_n].at = net_now_ms() + sim_latency + j;
    simq[simq_n].to = *to;
    simq[simq_n].len = len;
    memcpy(simq[simq_n].data, data, (size_t)len);
    simq_n++;
    sim_flush();
}

int net_recv(net_addr_t *from, void *buf, int max)
{
    if (sim_latency || sim_jitter || sim_loss) sim_flush();
    if (sock == BAD_SOCK) return -1;
    struct sockaddr_in a;
    socklen_t al = sizeof a;
    int n = (int)recvfrom(sock, (char *)buf, max, 0, (struct sockaddr *)&a, &al);
    if (n < 0) return -1;
    memcpy(from->ip, &a.sin_addr, 4);
    from->port = ntohs(a.sin_port);
    return n;
}

/* ---------------------------------------------------------------- */
/* This socket as seen from the internet (STUN) and from the LAN       */
/* ---------------------------------------------------------------- */

// A STUN binding request (RFC 5389) to a public server, from the socket
// the session will use, answers with the address and port the router gave
// it outside. Google's and Cloudflare's servers are free for this.
bool net_stun(net_addr_t *pub)
{
    static const struct { const char *host; int port; } servers[] = {
        { "stun.l.google.com", 19302 }, { "stun1.l.google.com", 19302 },
        { "stun.cloudflare.com", 3478 }, { "stun2.l.google.com", 19302 },
    };
    if (sock == BAD_SOCK) return false;
    if (getenv("KI2_STUN_FAKE")) {          // testing without internet: this machine's loopback
        net_addr_t l;
        if (!net_local(&l)) return false;
        *pub = l;
        pub->ip[0] = 127; pub->ip[1] = 0; pub->ip[2] = 0; pub->ip[3] = 1;
        return true;
    }
    uint8_t req[20] = { 0x00, 0x01, 0x00, 0x00, 0x21, 0x12, 0xa4, 0x42 };
    for (int i = 8; i < 20; i++) req[i] = (uint8_t)(rand() ^ ((int)net_now_ms() * 31 + i));
    for (int attempt = 0; attempt < 3; attempt++) {
        for (size_t k = 0; k < sizeof servers / sizeof servers[0]; k++) {
            net_addr_t to;
            if (net_resolve(servers[k].host, servers[k].port, &to)) raw_send(&to, req, sizeof req);
        }
        double until = net_now_ms() + 700;
        while (net_now_ms() < until) {
            uint8_t b[600];
            net_addr_t from;
            int n = net_recv(&from, b, sizeof b);
            if (n < 0) { SDL_Delay(10); continue; }
            if (n < 20 || b[0] != 0x01 || b[1] != 0x01 || memcmp(b + 4, req + 4, 16)) continue;
            int len = b[2] << 8 | b[3];
            for (int at = 20; at + 4 <= 20 + len && at + 4 <= n;) {
                int type = b[at] << 8 | b[at + 1], al = b[at + 2] << 8 | b[at + 3];
                const uint8_t *v = b + at + 4;
                if (at + 4 + al > n) break;
                if ((type == 0x0020 || type == 0x0001) && al >= 8 && v[1] == 0x01) {    // IPv4
                    bool x = type == 0x0020;
                    pub->port = (uint16_t)((v[2] << 8 | v[3]) ^ (x ? 0x2112 : 0));
                    for (int i = 0; i < 4; i++) pub->ip[i] = (uint8_t)(v[4 + i] ^ (x ? req[4 + i] : 0));
                    return true;
                }
                at += 4 + ((al + 3) & ~3);
            }
        }
    }
    return false;
}

// This socket on the local network: the address the way out goes through
// (no packet is sent for it) and the socket's own port.
bool net_local(net_addr_t *out)
{
    if (sock == BAD_SOCK) return false;
    struct sockaddr_in a;
    socklen_t al = sizeof a;
    if (getsockname(sock, (struct sockaddr *)&a, &al) != 0) return false;
    out->port = ntohs(a.sin_port);
    sock_t t = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (t == BAD_SOCK) return false;
    struct sockaddr_in d = { 0 };
    d.sin_family = AF_INET;
    d.sin_port = htons(53);
    d.sin_addr.s_addr = htonl(0x08080808);
    bool ok = connect(t, (struct sockaddr *)&d, sizeof d) == 0;
    struct sockaddr_in l;
    socklen_t ll = sizeof l;
    ok = ok && getsockname(t, (struct sockaddr *)&l, &ll) == 0;
    close_sock(t);
    if (ok) memcpy(out->ip, &l.sin_addr, 4);
    return ok;
}

// Keeps the router's mapping of this socket open while nothing else is sent.
void net_keepalive(void)
{
    static const uint8_t req[20] = { 0x00, 0x01, 0x00, 0x00, 0x21, 0x12, 0xa4, 0x42, 'C', 'B', 'k', 'e', 'e', 'p' };
    net_addr_t to;
    if (net_resolve("stun.l.google.com", 19302, &to)) raw_send(&to, req, sizeof req);
}
