# **TOI Project 1 - ESP + Raspberry Pi**
Internet-of-things zariadenie merajúce teplotu a svetlo založené na ESP32. Zariadenie komunikuje pomocou MQTT. Raspberry pi je využívané ako MQTT broker a zároveň ako gateway pre prístup na server založený na [thingsboard.io](https://thingsboard.io/).

# Setup
## Inštalácia a stiahnutie požiadavkov
Pre varzovanie projektu je využívany git, keďže je potrebné projekt odovzdať samostante ako zip, tak nieje možné ponechať originálne závistlosti pre využité knižnice. Odkazy na použité repozitáre sa nachádzajú v súbore `.gitmodules`, ide o dve knižnice a to pre one wire bus a využité tepelné čidlo

Pre stiahnutie podmodulov je potrebné spustiť 2 príkazu a to:

```bash
git submodule init
git submodule update
```

Ak by príkazy nefungovali tak je potrebné knižnice stiahnuť manuálne do priečinku `components` z odkazov:
- [https://github.com/DavidAntliff/esp32-owb.git](https://github.com/DavidAntliff/esp32-owb.git)
- [https://github.com/DavidAntliff/esp32-ds18b20.git](https://github.com/DavidAntliff/esp32-ds18b20.git)

V priečinok `components` by následne mal mať štruktúru:

- `components/`
  - `esp32-ds18b20/`
  - `esp32-owb/`


## Raspberry Pi OS
Využitie **Raspberry Pi Imager** z linku [https://www.raspberrypi.com/software/](https://www.raspberrypi.com/software/) , odporúčanie je nainštalovať 64 bitovú Lite verziu operačného systému Raspbian. Testované konkrétne pre verziu `arm64` z dňa `2022-01-28`.
## Raspberry Pi device tree overlay (optional)
V repozitári sa zároveň nachádza script `presetup.sh`, pomocou ktorého je možné nastaviť na Raspberry Pi 4 overlay pre nastavenie USB-OTG portu (USB-C) ako Ethernet Gadget. Toto umožní pripojenie Rasoberry Pi ku hosťovskému počíttaťu pomocou virtuálnej sieťovej karty.

Skript zároveň nakopíruje do domovského adresára uživateľa `/home/pi` súbor `rpisetup.sh` ktorý slúži k nainštalovanie iných požadovaných aplikácii ako je napríklad docker.

Návod na použitie `presetup.sh` scriptu 

**PO POUŽTÍ JE POTREBNÉ BEZPEČNE ODSTRÁNIŤ PRIPOJENÝ DISK**:
```
presetup.sh BOOT_DIR ROOT_DIR
    BOOT_DIR - mountpoint of RPi boot sector 
    ROOT_DIR - mountpoint of RPi root sector
```

Následne je možné Raspberry Pi pripojiť pomocou USB-C káblu priamo ku hosťovskému PC, ktoré bude zároveň slúžiť aj ako zdroj. Repozitár zároveň obsahuje utilitu `ip.sh` pomocou ktorej je možné nakonfigurovať ip adresu virtuálnej sieťovej karty a prípadne nastaviť `ip_tables` pre využitie ako hosťovského počítača ako **NAT**.

Návod na použitie `ip.sh` scriptu:

```
ip.sh forward-on|forward-off|ip-on|ip-off DEVICE
    forward-on  - set NAT iptable rules
    forward-off - delete NAT iptable rules
    ip-on       - set ip address of interface
    ip-off      - delete ip address of interface
    DEVICE      - Network device
```

## Raspberry Pi inštalácia **Docker**
Pre inštaláciu docker na **Raspberry Pi** je možné využit priložený *convenience* script ktorý sa nachádza v súbore `rpiscripts/rpisetup.sh`, skript **automaticky** zkopírovaný v domovskom adresáry pokiaľ bol **využitý presetup script**. V opačnom prípade je potrebné tento script na Raspberry Pi skopírovať manuálne napr. pomocou `scp`. Následne stačí script spustiť:
```
./rpisetup.sh install|docker|hotspot
    install - install docker, docker compose and reboot
    docker - setup docker environment
    hotspot - setup hostspot
```
Je možné vybrať z troch inštalačných fáz.
V prvej fáze sa nainštaluje `docker` a `docker-compose`, Raspberry Pi sa následne reštartuje. V druhej fáze prebieha nastavenie pracovného prostredia docker pre **MQTT broker** a **Python**. Posledná možnosť slúži na vytvorenie hotspotu, po zvolení tejto možnosti sa rpi reštartuje a defaut konfigurácia hotspotu bude:
```
dhcp-range=10.10.0.2,10.10.0.20,255.255.255.0
gateway=10.10.0.1
ssid=rpiIotGateway
wpa_passphrase=abcdefgh
wpa_key_mgmt=WPA-PSK
wpa=2
```
Hotspot bude fungovať na kanále 1.

## ESP **PlatformIO**

Pre Platformio je najlepšie nainštalovať Visual Studio Code package `platformio.platformio-ide` prípadne je možné využiť pip package pre platformio pomocou príkazu `pip install platformio`.

# Implementácia

## Sieť IoT
Pre mesh sieť na komunikáciu medzi ESP uzlami sme použili knižnicu ESP-NOW. Jedno zariadenie je nutné zvoliť ako *master* uzol odkomentovaním:
`#define CONFIG_IS_GATEWAY`
Mac adresu je nutné definovať v zdrojovom súbore **configuration.h**:
`const static uint8_t master_mac[] = { 0xXX,0xXX,0xXX,0xXX,0xXX,0xXX };`
Ostatné zariadenia, nie je nutné manuálne pridávať (špecifikovať *mac*), zariadenia sa automaticky pridávajú. Všetky zariadenia komunikujú na wifi-channel 7.

Uzol **master** sa pripojí na hotspot na RPi a následne komunikuje s RPi pomocou MQTT. Zvyšné zariadenia posielajú dáta zo senzorov na *master* každých 5 sekúnd, pomocou ESP-NOW. Master uzol následne preposiela prijaté dáta cez MQTT na RPi. Zároveň sám posiela dáta z vlastných senzorov (tiež každých 5 sekúnd). Správy sú minimálne, reprezetnovné priamo štruktúrou a v jednej správe sa odosielajú obe hodnoty (teplota a intenzita svetla).

## Zber a spracovanie dát z ESP
Hodnoty z teplotného senzoru DS18B20 získavame zo zbernice one-wire z pinu 4, pomocou knižníc `owb.h`, `owb-rtm.h` a `ds18b20.h`. Inicializácia prebieha v `app_main()` a samotné získavanie pomocou volania `get_temperature()`. 

Hodnoty z fotorezistoru získavame cez ADC prevodník na pine 34 pomocou funkcie `get_light_intensity()`. Raw data sú prevádzané na hodnotu v lumenoch a vrátená, odpor fotorezistoru je počítaný ako:
```
RLDR = (R * (Vref - Vout))/Vout
Vref = 3.3V
R = 10kOhm
Vout - napätie na pine 34
```
Prevod napätia na lumeny je:
`lumens = 500/(RLDR/1000)`

Dáta sú agregované na RPi z každej minúty. Hodnoty `min`, `max`, `average`, `median` sú počítané pomocou funkcií knižnice `numpy` a odosielané pomocou MQTT na Thingsboard.

## Spracovanie dát na Raspberry Pi

Logika spracovávania a agregácie dát je implementovaná pomocou programovacieho jazyka Python. Pre spúšťanie  prostredia s potrebnýmy knižnicami sa využíva docker container špecifikovaný v súbore `Dockerfile`. Tento image využíva Python container ako základ. Následne sú pridávané knižnice ako patho-mqtt a numpy. Implementácia celej logiky agregácie je v súbore `app/app.py`. 

Script `app.py` je spúštaný v kontajnery. Script sa  pripojí na lokálny MQTT broker hostovaný v kontajnery `eclipse-mosquitto`. Broker sa využíva pre získavanie a následné publikovanie dát z ESP NOW siete. Pre spracovávanie prijatých správ sa script `app.py` prihlási na odoberanie všetkých topicov na ktoré ESP bude publikovať dáta. 

Topicy sú všeobecne označené ako `/esp/[číslo ESP peer]/[temp|light]`. Každá prijatá správa je následne uložená do globálneho slovníku. Slovník obsahuje názov topic a zoznam prijatých hodnôt. Po stanovenej dobe (1 minúta) sa z týchto zoznamov spočíta priemer, medián, max a min. Všetky tieto hodnoty pre každý topic a aj teplotu cpu sa následne odošlú v jednej správe na Thingsboard topic pre naše zariadenie. Jednotlivé zoznamy hodnôt sa zároveň v tomto kroku vyčistia, čím zaručujeme agregáciu hodnôt ktoré prišli na Raspberry Pi v 1 minútovom okne.

Meranie teploty CPU Raspberry Pi prebieha periodickým čítaním (každé 2 sekundy) hodnoty zo súboru `/sys/class/thermal/thermal_zone0/temp`. V docker compose bolo potrebné vytvoriť mount point pre priečinok `/sys`, keďže tento súbor štandardne z kontaineru nieje prístupný.

Na Thingsboard sa následne pošle MQTT správa v **JSON** formáte:

```json
{
    /esp[číslo ESP peer]/temp/average: 12.3,
    /esp[číslo ESP peer]/temp/median: 12.2,
    /esp[číslo ESP peer]/temp/min: 1.23,
    /esp[číslo ESP peer]/temp/max: 23.4,
    /esp[číslo ESP peer]/light/average: 0.12,
    /esp[číslo ESP peer]/light/median: 0.123,
    /esp[číslo ESP peer]/light/min: 0.01,
    /esp[číslo ESP peer]/light/max: 12.3,
    ...
    ...
    ...
    /cpu/temp/average: 52.6,
    /cpu/temp/median: 53.2,
    /cpu/temp/min: 40.0,
    /cpu/temp/max: 65.8,
}
```

Kde hodnota `[číslo ESP peer]` značí poradové číslo zariadenia ESP, štandarde bude hodnota 1 patriť ROOT zariadeniu (zariadenie pripojené na hotspot a komunikujúce s Raspberry Pi). Ďaľšie čísla predstavujú postupne sa pripájajúce sa ostatné zariadenia v ESP NOW sieti.

## Thingsboard.io
Na serveri sme vytvorili užívateľa `Marek` a zariadenie `RPi-gateway`, ku ktorému je priradený. Token tohto zariadenia ďalej používame na komunikáciu MQTT medzi RPi a serverom. Na zariadenie prichádzajú dáta ako telemetrie pod rôznymi kľúčami (topicy z RPi). Podrobné štruktúra je v predošlej kapitole:

Tieto hodnoty sú ukladané a spracovávané pomocou `rule chain`, v ktorom prebieha prevod stupňov celsia na farenheity vzťahom: 
`Tf = (Tc * 1.8) + 32` 
V prípade chýbajúcej hodnoty je generovaný alarm, v ktorom sa nachádzajú informácie o chýbajúcich hodnotách.

Spracovnané dáta sú zobrazované v dashboarde `Log` ako kombinované čiarové grafy. V dashboarde sa taktisto zobrazujú aj aktuálne upozornenia (alarmy).

## Screenshoty z riešenia

V root adresáry repozitára sa zároveň nachádzajú 3 screenshoty v súboroch:
- `screen-dockerlog.png` - Ukážka docker logu pri posielaní a príjímaní správ na Raspberry Pi
- `screen-esplog.png` - Ukážka logovania stavu z ESP
- `screen-tihngsboard.png` - Ukážka dashboardu v Thingsboard

# Zaujímavosti a detaily

## Raspberry Pi, umelá záťaž a kde ju nájsť

V priečinku app sa zároveň nachádza script `big_mat.py` ktorý vytvára umelú záťaž pre testovanie merania teploty na Raspberry Pi. Script počíta násobenie dvoch veľkých náhodných matíc v nekonečnej slučke.

## Manuálny rebuild docker containerov na Raspberry Pi

Pri vývoji, alebo pre manuálne spustenie containerov je štanderdne potrebné spustiť znovu zostavenie kontajnerov a ich následné spustenie. Celý proces stojí na využití **docker compose v2**  pre aktualizovanie a spostenie potrebných kontainerov je následne potrebné spustiť príkazy v root repozitáru v poradí:

```bash
docker compose down
docker compose build
docker compose up
```