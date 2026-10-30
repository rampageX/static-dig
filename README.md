# static-dig

[![Build and release static dig](https://github.com/rampageX/static-dig/actions/workflows/static-dig.yml/badge.svg)](https://github.com/rampageX/static-dig/actions/workflows/static-dig.yml)
[![Windows native x64 (experimental)](https://github.com/rampageX/static-dig/actions/workflows/windows-dig-experimental.yml/badge.svg?branch=experiment%2Fwindows-native)](https://github.com/rampageX/static-dig/actions/workflows/windows-dig-experimental.yml)

**ISC BIND 9 `dig`：完全静态链接、单文件、跨 Linux 发行版运行。**

只构建 DNS 查询工具 `dig`，不构建 DNS 服务器 `named`。支持普通 DNS、DNSSEC、DNS-over-TLS (DoT) 和 DNS-over-HTTPS (DoH)。采用 musl libc，不依赖目标系统安装 BIND、OpenSSL 或 nghttp2 的动态库。

**English:** Standalone, fully static musl builds of ISC BIND 9 `dig` for ARMv5TE soft-float and portable x86_64. DNS, DNSSEC, DoT and DoH are enabled. `named` and runtime-loadable plugins are **not** built. See [Releases](https://github.com/rampageX/static-dig/releases) for binaries and SHA256 checksums.

## 下载与架构

从 [Releases](https://github.com/rampageX/static-dig/releases) 下载与 CPU 对应的文件及 `SHA256SUMS`：

| 架构 | 发布文件 | 适用场景 |
| --- | --- | --- |
| ARMv5TE / ARM EABI5 / soft-float | `dig_armv5te_static_9.20.X` | 老款 Tomato/OpenWrt Linux ARM 路由器；能够执行 32 位 ARM 程序的 ARMv7、ARM64 Linux |
| x86_64 / x86-64-v1 | `dig_x86_64_static_9.20.X` | x86_64 Linux VPS、服务器、Alpine/Debian/Ubuntu 等 |

ARMv5TE 版本已在 RT-AC5300 (Tomato)、N1、部分 ARM64 VPS 上实测。**ARM64 内核必须启用 32 位 AArch32/compat 执行支持**；并非所有 ARM64 环境都能运行 32 位 ELF。不能运行时需单独的 aarch64 原生构建。

每次发布生成两个可执行文件和校验清单：

```sh
sha256sum -c SHA256SUMS
chmod +x dig_*_static_*
./dig_x86_64_static_9.20.X -v     # 改成下载的真实版本/架构文件名
```

### 查询示例

把下方的 `dig` 换成刚下载的文件名，或者自行安装到 `$PATH`。

```sh
# 普通 DNS
dig @223.5.5.5 example.com A

# DoT：TLS 证书链与域名均验证
dig @223.5.5.5 example.com A +tls +tls-ca +tls-hostname=dns.alidns.com +time=3 +tries=1

# DoH：HTTP/2 /dns-query；TLS 证书链与域名均验证
dig @223.5.5.5 example.com A +https=/dns-query +tls-ca +tls-hostname=dns.alidns.com +time=3 +tries=1
```

`+tls-ca` 依赖目标机器上可读取的 CA 证书。x86_64 构建的 OpenSSL 默认路径为 `/etc/ssl`；ARMv5TE 为 Tomato 兼容而使用 `/opt/etc/ssl`。需要时可显式设置 `SSL_CERT_FILE=/path/to/ca-certificates.crt`。不要为了省事关闭证书校验。

## Windows x64（实验性预览）

**Windows 原生 `dig.exe` 9.20.30 已编译成功，并在 Windows Server 2025 x64 GitHub Runner 上通过实机查询验证。** 面向 Windows 10/11 x64（仍建议用户在自己的 Windows 机器上复测）。这是与 Linux musl 静态 ELF 完全不同的构建路线：用 MinGW-w64/UCRT 编译 BIND 源码，静态链接 OpenSSL、libuv、nghttp2 等第三方依赖，保留 Windows 系统自带的 DLL 调用。

- **[下载已通过测试的 Windows x64 dig.exe（GitHub Actions Artifact）](https://github.com/rampageX/static-dig/actions/runs/41044124400/artifacts/12972021558)**。需要登录 GitHub 下载，压缩包内为单个 `dig.exe`。实验版不是正式 Release，Actions Artifact 有保留期限。
- 版本目前固定为 **BIND 9.20.30**，尚未接入 Linux 自动升级/正式发布流程；未来 BIND 上游 API 更新可能需要移植补丁。
- 独立 `dig.exe`，无需另外安装 OpenSSL、BIND、MSYS2 运行库。依赖的 UCRT/Win32 DLL 由 Windows 提供；**DoT/DoH 的 `+tls-ca` 使用 Windows 系统 ROOT 信任库，保持证书链与服务器名称校验**。实验版先与现有旧版并存，不建议直接覆盖。
- **注意：**Windows 版需要 Windows x64；Linux 的 ARMv5TE / x86_64 ELF 不能作为 Windows 程序运行。

Windows PowerShell 测试命令：

```powershell
.\dig.exe -v
.\dig.exe '@223.5.5.5' example.com A +time=3 +tries=1
.\dig.exe '@223.5.5.5' example.com A +tls +tls-ca +tls-hostname=dns.alidns.com +time=3 +tries=1
.\dig.exe '@223.5.5.5' example.com A +https=/dns-query +tls-ca +tls-hostname=dns.alidns.com +time=3 +tries=1
```

PowerShell 中将 `@223.5.5.5` 放在引号内，可避免 `@` 被当作特殊语法。Windows 版通过 Windows 系统根证书库验证 TLS，默认不需要附加 PEM 文件。指定自定义 CA 时仍可使用 `+tls-ca=文件路径`；DoT/DoH 成功与否还取决于本机网络与远端服务器。安装前先通过 `where.exe dig.exe` 检查已有旧版的 PATH 优先级。

## 自动检测与发布

GitHub Actions 工作流：[`.github/workflows/static-dig.yml`](.github/workflows/static-dig.yml)。

1. 每天北京时间 **09:17** 检查 [ISC 官网下载页](https://www.isc.org/download/)上明确标注为 **Current Stable + ESV** 的 **BIND 9.20.x** 版本，不追踪开发版 9.21.x。
2. 如果该版本已存在 GitHub Release，就不重复构建/覆盖；如无 Release，则下载 ISC 官方源码与 `.asc` 签名，使用 ISC 官方公钥以及[固定的签名者指纹白名单](ci/release_source.py)验证，取得经过认证的源码 SHA256。
3. 两个独立 Runner 并行编译：ARMv5TE 通过固定 SHA256 的 Linux x86_64 → ARM musl 工具链交叉编译；x86_64 使用 `musl-gcc`。两条路线都强制完全静态链接。
4. 两个产物必须通过 ELF 静态检查、版本检查以及本地 UDP 模拟 DNS 查询。ARM 产物在 CI 上通过 `qemu-arm` 执行上述功能测试。
5. **全部通过**才创建 `v9.20.X` Release，一次上传两个二进制、`SHA256SUMS` 和源码来源信息；失败时不会发布半成品。

进入 GitHub 仓库的 **Actions → Build and release static dig → Run workflow** 可以手工触发：

- `bind_version` 留空：使用 ISC 当前稳定 ESV 9.20.x；填 `9.20.31`：验证并构建该指定版本（前提是 ISC 已正式发布并签名）。
- `build_existing` 设为 `true`：仅重新编译已发布的版本进行验证，**不会覆盖既有 Release 文件**。

推送构建脚本或 CI 文件到 `main`，以及提交 PR，也会执行验证构建；PR 不会自动发布。若新的 BIND 改变 `configure` 检查或静态链接 API，构建会失败并等待维护者修复，**绝不跳过失败强行发布**。

> 自动升级目前限定 **BIND 9.20.x ESV**。未来 ISC 更换稳定分支（例如 9.22.x），须检查兼容性、更新允许版本范围并重新验证后才能启用，不会悄悄切换大版本。

## 手工编译

两个脚本位于仓库根目录：

- [`bind_dig_armv5te_static.sh`](bind_dig_armv5te_static.sh)
- [`bind_dig_x86_64_static.sh`](bind_dig_x86_64_static.sh)

### x86_64（Alpine Linux）

```sh
apk add bash curl tar xz bzip2 coreutils build-base linux-headers \
    perl autoconf automake libtool pkgconf binutils

bash bind_dig_x86_64_static.sh --jobs 4
```

在 Ubuntu/Debian 上还可使用 `musl-tools` 提供的 `musl-gcc`（不能用 glibc GCC 冒充 musl）。输出默认位于 `/root/src/x86_64_result/`。

### ARMv5TE（Alpine Linux 交叉编译）

```sh
apk add bash curl tar xz bzip2 coreutils build-base linux-headers \
    perl autoconf automake libtool pkgconf binutils

# 需预先准备 arm-linux-musleabi-{gcc,ar,ranlib,strip,readelf} 等工具链。
CROSS_BIN_DIR=/opt/cross/bin bash bind_dig_armv5te_static.sh --jobs 4
```

输出默认位于 `/root/src/armv5te_result/`。`ROOT` 可以修改工作根目录；各架构的静态依赖安装在隔离的 `*_bind_dev/` 目录中，不影响系统库。`--print` 查看配置，`--rebuild` 重新构建，`--no-doh` 关闭 DoH（正式 CI 产物不关闭）。

本地脚本默认采用 `192.168.2.8:7373` 作为**仅在没有环境代理时**的回退代理，适合作者本地环境。其他环境设置 `PROXY_FALLBACK=''` 即完全禁用回退；CI 已这样设置。已有 `http_proxy`/`https_proxy` 环境变量仍优先使用。

### 编译参数和限制

目前手动脚本固定经过实测的依赖版本：OpenSSL **3.5.9**、libuv **1.53.0**、Userspace-RCU **0.15.7**、nghttp2 **1.70.0**，所有源码包均做 SHA256 校验。默认编译 BIND **9.20.30**；更新 BIND 版本必须同时提供正确的、经过独立认证的 `BIND_SHA256`：

```sh
BIND_SHA256='<verified SHA256 of ISC-signed tarball>' \
  bash bind_dig_x86_64_static.sh --version 9.20.31 --jobs 4
```

ISC 上游明确禁止对完整 BIND 套件进行静态链接（会影响 `dlopen()` 等功能）。两份脚本只针对独立 `dig` 目标绕过全套件的 `configure` 限制，并避免不属于 `dig` 的 `libcap` 依赖。旧 ARM 内核使用 `liburcu-mb` 避免 `membarrier` 要求；x86_64 则包含在必要时修复 `quota.c` 的原子类型宽度兼容补丁。这些更改**不适用于 `named`**。

## 验证记录

- **2026-10-30，ARMv5TE**：32 位 ARM EABI5 静态 ELF，Tomato RT-AC5300 上 `dig -v`、DoH HTTPS 查询成功；N1 / 若干 ARM64 VPS 可运行。
- **2026-10-30，x86_64**：64 位 x86 静态 ELF，在阿里云 VPS 实测 `DiG 9.20.30`、DoT `:853`（40 ms）与 DoH `:443`（100 ms）成功，均返回 `NOERROR`。
- **2026-10-30，Windows x64（实验分支）**：[GitHub Actions Windows Server 2025 实机日志](https://github.com/rampageX/static-dig/actions/runs/41044124400)：`dig.exe -v` → `DiG 9.20.30`；本地 UDP DNS 查询返回 `203.0.113.9` 和 `NOERROR`；Cloudflare `1.1.1.1:853` 的 DoT 与 `1.1.1.1:443` 的 DoH 均在 `+tls-ca +tls-hostname=cloudflare-dns.com` 下返回 `NOERROR`。Windows 10/11 用户侧尚待复核。
- **Linux 自动 CI**：离线协议检查和签名校验作为每次正式发布前的硬门槛；真实公网 DoT/DoH 仍建议在实际部署目标上复核，因为各机器的 CA 文件和网络策略不同。

官方资料：[ISC BIND 下载](https://www.isc.org/download/) · [ISC 源码签名验证说明](https://kb.isc.org/docs/aa-01225) · [ISC 代码签名公钥及指纹](https://www.isc.org/pgpkey/)

项目独立于 ISC；二进制及编译脚本均不代表 ISC 官方提供或支持。
