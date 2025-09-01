
# macos:
```bash
# Install basic libusb (you may already have this)
brew install libusb

# For USBasp specifically:
brew tap osx-cross/avr
brew install avrdude
```

# LED USB Control avec ATtiny85 et V-USB

Ce projet implémente une communication USB sur un ATtiny85 en utilisant la librairie V-USB pour contrôler une LED NeoPixel via USB.

## Matériel requis

- ATtiny85
- LED NeoPixel (WS2812B)
- 2 diodes Zener 3.6V (pour les lignes D+ et D-)
- 2 résistances de 68Ω
- 1 résistance de 1.5kΩ (pull-up)
- 1 résistance de 220Ω à 1kΩ (pour la LED)
- Condensateur 100nF (découplage)

## Schéma de câblage

```
ATtiny85 Pinout:
     ┌───────────┐
VCC  │1  ┌───┐ 8│ VCC
PB3  │2  │   │ 7│ PB2 (LED NeoPixel)
PB4  │3  └───┘ 6│ PB1
GND  │4       5│ PB0
     └───────────┘
```

### Connexions USB:

- **USB D-** → PB3 (pin 2) via résistance 68Ω + diode Zener 3.6V vers GND
- **USB D+** → PB4 (pin 3) via résistance 68Ω + diode Zener 3.6V vers GND
- **Pull-up**: Résistance 1.5kΩ entre PB4 (D+) et VCC
- **USB GND** → GND
- **USB VCC** → VCC (si alimentation via USB)

### Connexion LED:
- **LED Data** → PB2 (pin 7) via résistance 220Ω
- **LED VCC** → VCC
- **LED GND** → GND

### Condensateur de découplage:
- 100nF entre VCC et GND, proche du microcontrôleur

## Configuration

Le projet utilise les paramètres suivants:
- **Fréquence**: 16.5 MHz (oscillateur interne calibré)
- **USB Low-Speed**: Conforme aux spécifications USB
- **Protocol**: HID (Human Interface Device)
- **Vendor ID**: 0x16c0 (VID libre d'usage)
- **Product ID**: 0x05df (PID libre d'usage)

## Compilation et téléversement

### Référence V-USB
https://github.com/obdev/v-usb

### À faire pour programmer la puce: 

1. **Configurer les fuses** (dans le menu PlatformIO de VSCode):
   - Platform/Set Fuses. Si erreur, vérifier la connexion de la pince

2. **Compiler et téléverser**:
   - General/Build : Compile le programme
   - General/Upload : Upload le programme dans le microcontroller

3. **Ou en ligne de commande**:
   ```bash
   pio build
   pio run -t upload
   ```

## Configuration des fuses

Les fuses sont configurés automatiquement dans `platformio.ini`:
- **LFUSE**: 0xE1 (oscillateur interne 16 MHz, sans division)
- **HFUSE**: 0xDD (paramètres par défaut)
- **EFUSE**: 0xFF (BOD désactivé)

## Test de la communication

### Installation des dépendances Python:
```bash
pip install hidapi
```

### Test avec le script Python:
```bash
python usb_test.py
```

Le script permet de:
- Détecter automatiquement le device USB
- Tester différentes couleurs préprogrammées
- Contrôler manuellement la couleur RGB

### Utilisation du script:
```
RGB> 255 0 0      # LED rouge
RGB> 0 255 0      # LED verte
RGB> 0 0 255      # LED bleue
RGB> 255 255 255  # LED blanche
RGB> 0 0 0        # LED éteinte
RGB> quit         # Quitter
```

## Protocol de communication

Le device utilise le protocol HID avec des rapports de 8 bytes:

### Envoi de données (Host → Device):
- Byte 0: Rouge (0-255)
- Byte 1: Vert (0-255)  
- Byte 2: Bleu (0-255)
- Bytes 3-7: Réservés

### Réception de données (Device → Host):
- Byte 0: Rouge actuel
- Byte 1: Vert actuel
- Byte 2: Bleu actuel
- Byte 3: Status (0x00 = OK)
- Bytes 4-7: Réservés

## Dépannage

### Device non détecté:
1. Vérifier le câblage USB (D+, D-, GND, VCC)
2. Vérifier les diodes Zener et résistances
3. Vérifier la fréquence du microcontrôleur (16.5 MHz)
4. Vérifier les fuses

### LED ne s'allume pas:
1. Vérifier la connexion sur PB2
2. Vérifier l'alimentation de la LED (5V recommandé)
3. Vérifier la résistance de protection

### Communication instable:
1. Ajouter un condensateur de découplage plus gros (10µF)
2. Vérifier la qualité des connexions
3. Utiliser des fils courts pour les signaux USB

## Références

- [V-USB Documentation](https://www.obdev.at/products/vusb/index.html)
- [ATtiny85 Datasheet](https://www.microchip.com/wwwproducts/en/ATtiny85)
- [USB HID Class Specification](https://www.usb.org/sites/default/files/documents/hid1_11.pdf)
- [NeoPixel Guide](https://learn.adafruit.com/adafruit-neopixel-uberguide)