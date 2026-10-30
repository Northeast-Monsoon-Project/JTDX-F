# JTDX-F NTP 校时

在“设置 → 常规”勾选 NTP 校时，完成系统权限提示，选择服务器后点确定。关闭时不使用 NTP；开启后立即尝试校时，此后默认每 15 分钟一次，失败后 1 分钟重试。发射/PTT 时暂缓，停止发射后继续。系统权限被拒绝时不会更改时钟，可重新勾选再次申请。

校时间隔可选 1、5、10、15、30 分钟，以及 1、3、6、9、12 小时。默认 15 分钟；手动选择会保存并在下次启动时恢复。栏目标题为“NTP 校时（勾选后会自动申请管理员权限）”，关闭时服务器和间隔控件一起灰显。

## UTC 与用户时区

NTP 时间是 UTC。Windows 使用 UTC 的 `SetSystemTime`；Linux 使用 Unix 时间戳调用 `clock_settime(CLOCK_REALTIME)`。程序不改时区、不对 NTP 值加减本地时差。用户桌面继续按原时区显示当地时间，软件通联时间和日志继续使用 UTC。测试覆盖 UTC 午夜跨日期以及 UTC+8、UTC−4 显示同一瞬间；NTP 秒计数的 2036 年回绕也有测试。

## 预设列表

共 50 个地址，亚洲排在最前；欧洲 5 国 5 家，非洲 3 国 5 家，南美 4 国 5 家；日本 3 家，韩国 1 家；另含北美、澳大利亚、新西兰及全球服务。每个服务商最多 2 个地址；只公开一个地址的服务商列 1 个，不虚构备用地址。“其他”可输入自定义域名或 IPv4/IPv6 地址。

| 地区 / 服务商 | 地址 | 来源 |
|---|---|---|
| 亚洲 / 中国 / 阿里云 | `ntp1.aliyun.com`、`ntp2.aliyun.com` | [服务说明](https://developer.alibaba.com/docs/doc.htm?articleId=112525&docType=1&treeId=608) |
| 亚洲 / 中国 / 腾讯云 | `ntp.tencent.com`、`ntp1.tencent.com` | [服务说明](https://intl.cloud.tencent.com/zh/document/product/213/32379) |
| 亚洲 / 中国 / 中科院国家授时中心 | `ntp.ntsc.ac.cn` | 用户指定 |
| 亚洲 / 中国 / 百度 | `ntp1.baidu.com` | 用户指定 |
| 亚洲 / 中国 / NTP Pool | `cn.pool.ntp.org` | [服务说明](https://www.ntppool.org/zone/cn) |
| 亚洲 / 中国 / 清华 TUNA | `ntp.tuna.tsinghua.edu.cn` | [服务说明](https://tuna.moe/help/ntp/) |
| 亚洲 / 日本 / NICT | `ntp.nict.jp` | [服务说明](https://www.nict.go.jp/en/sts/ntp.html) |
| 亚洲 / 日本 / Internet Multifeed | `ntp1.jst.mfeed.ad.jp`、`ntp2.jst.mfeed.ad.jp` | [服务说明](https://www.mfeed.ad.jp/ntp/faq/) |
| 亚洲 / 日本 / 樱花网络 | `ntp1.sakura.ad.jp` | [服务说明](https://manual.sakura.ad.jp/cloud/support/ntp.html) |
| 亚洲 / 韩国 / KRISS | `ntp.kriss.re.kr` | [服务说明](https://www.kriss.re.kr/board.es?act=view&bid=0002&list_no=63463&mid=a10503000000&nPage=8) |
| 亚洲 / Apple | `time.asia.apple.com` | 用户指定 |
| 欧洲 / 德国 / PTB | `ptbtime1.ptb.de`、`ptbtime2.ptb.de` | [服务说明](https://www.ptb.de/cms/fileadmin/internet/publikationen/broschueren/About_Time_2013.pdf) |
| 欧洲 / 英国 / NPL | `ntp1.npl.co.uk`、`ntp2.npl.co.uk` | [服务说明](https://resource.npl.co.uk/docs/science_technology/time_frequency/products/user_guide_its.pdf) |
| 欧洲 / 瑞典 / Netnod | `gbg1.ntp.netnod.se`、`gbg2.ntp.netnod.se` | [服务说明](https://www.netnod.se/ntp/connect-to-ntp-servers) |
| 欧洲 / 波兰 / GUM | `tempus1.gum.gov.pl`、`tempus2.gum.gov.pl` | [服务说明](https://gum.gov.pl/en/services/time-service/187%2CTime-Service.html) |
| 欧洲 / 法国 / RDEM Systems | `ntp-1.rdem-systems.com`、`ntp-2.rdem-systems.com` | [服务说明](https://ntp.rdem-systems.com/status.php) |
| 非洲 / 南非 / SANReN | `tick.sanren.ac.za`、`tock.sanren.ac.za` | [服务说明](https://tick.sanren.ac.za/) |
| 非洲 / 南非 / INX-ZA | `ntp1.inx.net.za`、`ntp2.inx.net.za` | [服务说明](https://wiki.inx.net.za/display/pub/INX%2BServices) |
| 非洲 / 南非 / NMISA | `time.nmisa.org` | [服务说明](https://support.ntp.org/Servers/PublicTimeServer001771) |
| 非洲 / 肯尼亚 / icolo | `ntp0.icolo.io`、`ntp1.icolo.io` | [服务说明](https://www.peeringdb.com/net/14510) |
| 非洲 / 毛里求斯 / Mauritius Telecom | `ntp.telecom.mu` | [服务说明](https://www.ntppool.org/a/mauritiustelecom) |
| 北美洲 / 美国 / NIST | `time.nist.gov` | [服务说明](https://tf.nist.gov/tf-cgi/servers.cgi/en-en/) |
| 北美洲 / 加拿大 / NRC | `time.nrc.ca`、`time.chu.nrc.ca` | [服务说明](https://www.nrc.canada.ca/en/certifications-evaluations-standards/canadas-official-time/network-time-protocol-ntp) |
| 南美洲 / 巴西 / NIC.br | `a.ntp.br`、`b.ntp.br` | [服务说明](https://ntp.br/faq/) |
| 南美洲 / 巴西 / 国家天文台 | `200.20.186.75`、`200.20.186.94` | [服务说明](https://www.gov.br/observatorio/pt-br/assuntos/areas-de-atuacao/tempo-e-frequencia/acerte-seu-computador-via-internet) |
| 南美洲 / 智利 / SHOA | `ntp.shoa.cl` | [服务说明](https://shoabucket.s3.amazonaws.com/horaoficial.cl/procedimiento/procedimiento_ntp.pdf) |
| 南美洲 / 哥伦比亚 / INM | `ntp1.inm.gov.co`、`ntp2.inm.gov.co` | [服务说明](https://inm.gov.co/nuevo-acceso-para-la-sincronizacion-con-la-hora-legal-para-la-republica-de-colombia/) |
| 南美洲 / 阿根廷 / INTI | `ntp.inti.gob.ar` | [服务说明](https://www.inti.gob.ar/hora-inti) |
| 大洋洲 / 澳大利亚 / NTP Pool | `au.pool.ntp.org` | [服务说明](https://www.ntppool.org/zone/au) |
| 大洋洲 / 新西兰 / MSL（限新西兰网络） | `pool.msltime.measurement.govt.nz` | [服务说明](https://www.measurement.govt.nz/about-us/official-new-zealand-time/about-time) |
| 全球 / Microsoft Windows | `time.windows.com` | [服务说明](https://learn.microsoft.com/en-us/windows-server/networking/windows-time-service/accurate-time) |
| 全球 / Apple | `time.apple.com` | 用户指定 |
| 全球 / Google | `time1.google.com`、`time2.google.com` | [服务说明](https://developers.google.com/time/guides) |
| 全球 / Cloudflare | `time.cloudflare.com` | [服务说明](https://developers.cloudflare.com/time-services/ntp/) |

韩国 KRISS 已宣布从 2026 年 2 月改用 `ntp.kriss.re.kr`，因此没有收录已停用的 `time.kriss.re.kr` / `time2.kriss.re.kr`。清华 TUNA 对校内使用说明了校园网认证要求；新西兰 MSL 仅接受新西兰网络连接。部分授时机构仅公布 IP 地址，按其官方说明原样列入。服务可用性取决于运营方和用户网络，域名解析成功不代表 UDP 授时已验证。

NTP Pool 的中国与澳大利亚池只供用户主动选择，软件默认服务器仍为阿里云，且 NTP 默认关闭；公开发布若改为默认使用 Pool，应按其 [vendor zone 规则](https://www.ntppool.org/en/vendors.html)申请专用区域。Google 使用 leap smear；软件一次只使用用户选定的单个服务器，不混合不同闰秒处理服务。

## 权限与退出

Windows 通过 UAC 启动 `jtdx-f-clock.exe`，Linux 通过 polkit/pkexec 启动 `jtdx-f-clock`；主界面保持普通权限。没有额外授予永久校时权限，也不创建开机服务。设置保持开启时，下次启动重新申请本次会话所需权限。操作系统可能按自身策略缓存授权，软件不会伪造权限提示。

辅助进程只接受经过本次随机令牌认证的本机连接，只能配置 NTP 服务器，不能执行任意命令，也不接受由主程序直接指定的任意时间。关闭选项、正常退出、主程序崩溃导致连接断开，或心跳停止 30 秒后，辅助进程退出。取消设置会恢复原配置。

只读诊断命令（不申请权限、不校时）：

```text
jtdx-f-clock --query ntp1.aliyun.com
```

Windows 对应程序名为 `jtdx-f-clock.exe`。成功时输出 UTC、偏差（毫秒）和网络往返延迟；失败会输出错误原因。

实现参考：[NTPv4 / RFC 5905](https://www.rfc-editor.org/rfc/rfc5905)、[Microsoft SetSystemTime](https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-setsystemtime)。
