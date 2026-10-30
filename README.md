# BIND 9.20.30 dig: x86_64 musl fully static

This is the x86_64 companion to the ARMv5TE-only build script. It builds **dig only**, not `named` or loadable modules, using the official BIND 9.20.30 sources and pinned SHA256 source archives.

## Build on native x86_64 Alpine Linux

```sh
apk add bash curl tar xz bzip2 coreutils build-base linux-headers \
    perl autoconf automake libtool pkgconf binutils

mkdir -p /root/src
cd /root/src
bash bind_dig_x86_64_static.sh --jobs 4
```

Output: `/root/src/x86_64_result/dig_x86_64_static_9.20.30`

Build caches: `/root/src/.downloads/bind-dig` and `/root/src/.build/x86_64/bind-dig`. Static dependencies are installed only into `/root/src/x86_64_bind_dev` (isolated from the ARM build).

Optional flags: `--print`, `--rebuild`, `--no-doh`, `--version 9.20.X` (the last requires an explicit `BIND_SHA256`). `ROOT` can override the default source root. The fallback proxy is `http://192.168.2.8:7373` when the environment has no proxy variables.

On Ubuntu/Debian build hosts, `musl-gcc` from the `musl-tools` package is supported, provided Linux UAPI headers are installed. The script detects a native Alpine musl GCC automatically and refuses a glibc-only compiler.

## Content and compatibility

- BIND 9.20.30 `dig`, with standard DNS, DNSSEC, DNS-over-TLS and DNS-over-HTTPS enabled.
- x86-64-v1 (`-march=x86-64 -mtune=generic`): no AVX or machine-specific instruction requirement.
- OpenSSL 3.5.9, libuv 1.53.0, Userspace-RCU 0.15.7, and nghttp2 1.70.0, linked statically.
- OpenSSL default CA directory `/etc/ssl` for mainstream Linux VPS distributions. Install `ca-certificates` on the VPS, or explicitly set `SSL_CERT_FILE` when needed.
- ISC's full-server static build guard, Linux `libcap` requirement, and the forced membarrier RCU variant are bypassed **only to build the standalone `dig` target**. An additional conditional fix handles a GCC/musl `uint_fast32_t` atomic-width mismatch in BIND's `quota.c`.
- **Do not** use the static-link guard bypass to build or deploy the full `named` server.

## Validation on the builder (2026-10-30)

1. Full build succeeded on Linux x86_64 with GCC 13.3 plus musl-gcc (musl 1.2.4); verified ELF64 x86-64, `statically linked`, stripped, no `INTERP` / `DYNAMIC` program headers.
2. `dig -v` printed `DiG 9.20.30` when run with process/network permissions.
3. A local UDP DNS server returned a synthetic `example.com A 203.0.113.7`; the binary queried it, parsed the answer, and displayed `NOERROR` correctly.
4. External DNS/DoT/DoH queries were **not** verified on this builder because outbound direct TCP/UDP port connections were refused by the test environment. HTTPS/nghttp2 support was retained in the compiled binary; please validate against a real resolver on your VPS.

Suggested VPS checks:

```sh
./dig_x86_64_static_9.20.30 -v
file ./dig_x86_64_static_9.20.30
./dig_x86_64_static_9.20.30 @223.5.5.5 example.com A +tls +tls-ca +tls-hostname=dns.alidns.com +time=3 +tries=1
./dig_x86_64_static_9.20.30 @223.5.5.5 example.com A +https=/dns-query +tls-ca +tls-hostname=dns.alidns.com +time=3 +tries=1
```

SHA256 of provided files:

```
1acd80058c2997fc139d2703b9a82524d0cd2a67e0e4617db3c260f731494121  bind_dig_x86_64_static.sh
9cc3c936bb2d1ba4294319caf67d41e9d50ac3219b971aed18e2cd22d3f1e4af  dig_x86_64_static_9.20.30
```
