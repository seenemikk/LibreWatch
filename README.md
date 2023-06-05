# LibreWatch

Welcome to LibreWatch, an open-source smartwatch that prioritizes privacy and customization. You can find all the hardware related files [here](https://github.com/seenemikk/LibreWatch-HW).

<p align="center">
    <img src="doc/images/librewatch.jpg" width="400" height="400">
</p>
 
# Features

* Smartphone related:
  * Displaying notifications and calls
  * Time synchronization
  * Over-the-air firmware updates
  * Media player control
* Step counter
* Stopwatch
* 2 months of battery life

Currently the smartphone related features only work with iOS devices, as there is no Android application for it yet.

# Getting Started

## Requirements

In order to install all the required libraries you should follow the [nRF Connect SDK Getting Started](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.3.0/nrf/getting_started.html) guide. You can follow either the manual or automatic installation guide, but I recommend following the automatic guide with [nRF Connect for Desktop](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.3.0/nrf/getting_started/assistant.html#gs-assistant), because installs all the necessary libraries for you.

## Cloning

```bash
west init -m https://github.com/seenemikk/LibreWatch.git
cd application
west update
```

## Building

You can build the project either on CLI or with VSCode. The following shows how to build it with VSCode. If you don't want to use VSCode, you can follow [nRF Connect SDK Getting Started](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.3.0/nrf/getting_started.html) on how to build it on CLI.

1. Launch VSCode and open nRF Connect extension window
2. Click on `Open an existing application` and open the cloned repository
3. In the `Applications` section click on `No build configurations` in order to create a new build configuration
   1. Under `Board` select `librewatch`
   2. Click `Build Configuration`
