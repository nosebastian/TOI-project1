# TOI Project 1 - ESP + Raspberry Pi 
Internet-of-thing zariadenie merajúce teplotu a svetlo založené na ESP32. Zariadenie komunikuje pomocou MQTT. Raspberry pi je využívané ako MQQT broker a zároveň ako gataway pre prístup na server založený na [thingsboard.io](https://thingsboard.io/). Možno niekedy bude aj GUI.

# Setup
## Raspberry Pi OS
Využitie **Raspberry Pi Imager** z linku [https://www.raspberrypi.com/software/](https://www.raspberrypi.com/software/) , odporúčanie je nainštalovať 64 bitovú Lite verziu operačného systému Raspbian. Testované konkrétne pre verziu `arm64` z dňa `2022-01-28`.
## Raspberry Pi device tree overlay (optional)
V repozitári sa zároveň nachádza script `presetup.sh`, pomocou ktorého je možné nastaviť na Raspberry Pi 4 overlay pre nastavenie USB-OTG portu (USB-C) ako Ethernet Gadget. Toto umožní pripojenie Rasoberry Pi ku hosťovskému počíttaťu pomocou virtuálnej sieťovej karty.

Skript zároveň nakopíruje do domovského adresára uživateľa `/home/pi` súbor `rpisetup.sh` ktorý slúži k nainštalovanie iných požadovaných aplikácii ako je napríklad docker.

Návod na použitie `presetup.sh` scriptu 

**PO PUŽTÍ JE POTREBNÉ BEZPEČNE ODSTRÁNIŤ PRIPOJENÝ DISK**:
```
presetup.sh BOOT_DIR ROOT_DIR
    BOOT_DIR - mountpoint of RPi boot sector 
    ROOT_DIR - mountpoint of RPi root sector
```

Následne je možné Raspberry Pi pripojiť pomocou USB-C káblu priamo ku hosťovskému PC, ktoré bude zároveň slúžiť aj ako zdroj. Repozitár zároveň obsahuje utilitu `ip.sh` pomocou ktorej je možné nakonfigurovať ip adresu virtuálnej sieťovej karty a prípadne nastaviť `ip_tables` pre využitie ako hosťouského počítača ako **NAT**.

Návod na použirie `ip.sh` scriptu:

```
ip.sh forward-on|forward-off|ip-on|ip-off DEVICE
    forward-on  - set NAT iptable rules
    forward-off - delete NAT iptable rules
    ip-on       - set ip address of interface
    ip-off      - delete ip address of interface
    DEVICE      - Network device
```

## Raspberry Pi inštalácia **Docker**
Pre inštaláciu docker je možné využit priložený ;*convenience* script ktorý sa nachádza pod `rpiscripts/rpisetup.sh`, skript bude v domovskom adresáry pokiaľ bol využítý predchádzajúci krok. Inak je potrebné tento script na raspberry pi skopírovať napríklad pomocou `scp`. Následne postačuje script spustiť `./rpisetup.sh 1|2`, kde je možné vybrať z dvoch inštalačných fáz.
V prvej fáze sa nainštaluje `docker` a `docker-compose`, Raspberry Pi sa následne reštartuje. V druhej fáze prebieha nastavenie praconvého prostredia aplikácií a teda **MQTT broker** a **Python**.

## ESP **PlatformIO**

Pre Platformio je najlepšie nainštalovať Visual Studio Code package `platformio.platformio-ide` prípadne je možné využiť pip package pre platformio pomocou príkazu `pip install platformio`