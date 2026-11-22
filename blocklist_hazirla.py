import requests
import hashlib
from pathlib import Path

# ==========================================
# AYARLAR
# ==========================================

LIMIT = 12000

OUTPUT = (
    Path.home()
    / "Desktop"
    / "blocklist.txt"
)

# Türkiye odakli ana liste
TURKISH_AD_HOSTS = (
    "https://raw.githubusercontent.com/"
    "symbuzzer/Turkish-Ad-Hosts/"
    "main/dns"
)

# Türkiye odakli ikinci liste
TURK_ADLIST = (
    "https://raw.githubusercontent.com/"
    "bkrucarci/turk-adlist/"
    "master/hosts"
)

# Genel filtre - kalan boslugu dolduracak
HAGEZI_LIGHT = (
    "https://cdn.jsdelivr.net/gh/"
    "hagezi/dns-blocklists@latest/"
    "wildcard/light-onlydomains.txt"
)


# ==========================================
# DOMAIN TEMIZLEME
# ==========================================

def clean_domain(line):

    line = line.strip().lower()

    if not line:
        return None

    # Yorum
    if line.startswith("#"):
        return None

    if line.startswith("!"):
        return None

    # Hosts formatlari:
    #
    # 0.0.0.0 example.com
    # 127.0.0.1 example.com

    if line.startswith("0.0.0.0"):
        parts = line.split()

        if len(parts) >= 2:
            line = parts[1]
        else:
            return None

    elif line.startswith("127.0.0.1"):
        parts = line.split()

        if len(parts) >= 2:
            line = parts[1]
        else:
            return None

    # IPv6 localhost
    elif line.startswith("::1"):
        return None

    # AdGuard format:
    #
    # ||example.com^

    if line.startswith("||"):
        line = line[2:]

    if line.endswith("^"):
        line = line[:-1]

    # URL olmamali
    line = line.replace(
        "http://",
        ""
    )

    line = line.replace(
        "https://",
        ""
    )

    line = line.strip(".")

    # IP adreslerini alma
    pieces = line.split(".")

    if all(
        piece.isdigit()
        for piece in pieces
    ):
        return None

    # Domain kontrolu
    if "." not in line:
        return None

    if (
        "/" in line
        or "*" in line
        or " " in line
        or "\t" in line
        or "^" in line
    ):
        return None

    return line


# ==========================================
# LISTE INDIR
# ==========================================

def download_list(name, url):

    print()
    print(
        f"{name} indiriliyor..."
    )

    response = requests.get(
        url,
        timeout=60
    )

    response.raise_for_status()

    domains = set()

    for line in response.text.splitlines():

        domain = clean_domain(line)

        if domain:
            domains.add(domain)

    print(
        f"{name}: "
        f"{len(domains)} domain"
    )

    return domains


# ==========================================
# SABIT DAGILIM PUANI
# ==========================================

def score(domain):

    return hashlib.sha256(
        domain.encode("utf-8")
    ).digest()


# ==========================================
# LISTELERI INDIR
# ==========================================

turkish_main = download_list(
    "Turkish-Ad-Hosts",
    TURKISH_AD_HOSTS
)

turkish_secondary = download_list(
    "turk-adlist",
    TURK_ADLIST
)

hagezi = download_list(
    "HaGeZi Light",
    HAGEZI_LIGHT
)


# ==========================================
# TURKIYE ONCELIKLI LISTE
# ==========================================

selected = []
used = set()


def add_domains(domains):

    global selected

    for domain in sorted(domains):

        if len(selected) >= LIMIT:
            return

        if domain in used:
            continue

        used.add(domain)
        selected.append(domain)


# 1.
# En guncel Türkiye odakli liste
# tamamen once gelir.
add_domains(
    turkish_main
)

turkish_main_count = len(selected)


# 2.
# Eski Turk-adlist'te olup
# birinci listede olmayanlari ekle.
before = len(selected)

add_domains(
    turkish_secondary
)

turkish_secondary_count = (
    len(selected) - before
)


# ==========================================
# KALAN ALANI HAGEZI ILE DOLDUR
# ==========================================

remaining = LIMIT - len(selected)

if remaining > 0:

    candidates = [
        domain
        for domain in hagezi
        if domain not in used
    ]

    # Listenin sadece alfabetik basini
    # secmek yerine butun listeden
    # dengeli secim.
    candidates.sort(
        key=score
    )

    for domain in candidates[:remaining]:

        used.add(domain)
        selected.append(domain)


hagezi_count = (
    len(selected)
    - turkish_main_count
    - turkish_secondary_count
)


# ==========================================
# DOSYAYA YAZ
# ==========================================

selected.sort()

with open(
    OUTPUT,
    "w",
    encoding="utf-8",
    newline="\n"
) as file:

    for domain in selected:

        file.write(
            domain + "\n"
        )


# ==========================================
# SONUC
# ==========================================

print()
print("==============================")
print("BLOCKLIST HAZIR")
print("==============================")

print(
    "Turkish-Ad-Hosts:",
    turkish_main_count
)

print(
    "turk-adlist ek:",
    turkish_secondary_count
)

print(
    "HaGeZi ek:",
    hagezi_count
)

print("------------------------------")

print(
    "TOPLAM:",
    len(selected)
)

print(
    "Dosya:",
    OUTPUT
)