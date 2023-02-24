# http-botbase

## A project heavily based on sys-botbase that allows remote control via HTTP

Wireless communication [is notoriously unreliable](https://www.researchgate.net/publication/4361162_Wireless_reliability_Rethinking_80211_packet_loss), and this sys-module attempts to offer a better solution solely based on wireless communication, to avoid packet loss using a well-established - albeit admittedly much slower - packet loss tolerant protocol: HTTP.
sys-botbase is a Nintendo Switch sys-module that allows the console to be remotely controlled via raw sockets - http-botbase does exactly the same, but uses HTTP on top of raw sockets, and is internally written in C++.

The API will be documented as soon as the sys-module is deemed stable enough for use.

This project is currently under active development, expect bugs and crashes.

## Credits

I would like to thank each and every single one who has helped me learn and develop this little project: berichan and all the members of their community on Discord, m4xw, SciresM, Behemoth, piepie62 & Anubis.

The following repositories were also extremely useful during development:

- [olliz0r@sys-botbase](https://github.com/olliz0r/sys-botbase)
- [tomvita@EdiZon-SE](https://github.com/tomvita/EdiZon-SE)
- [tomvita@Noexes](https://github.com/tomvita/Noexes)
- [yhirose@cpp-httplib](https://github.com/yhirose/cpp-httplib)
- [switchbrew@libnx](https://github.com/switchbrew/libnx)
- [zaksabeast@sys-http](https://github.com/zaksabeast/sys-http)

## License

[MIT](./LICENSE).
