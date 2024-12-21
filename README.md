# I2PChat

*Mammoth shit to make a secure chat with anon number 2.5*.

## Screenshots

![screenshot-roster](https://user-images.githubusercontent.com/19966907/82762245-7cf1db80-9e32-11ea-9c0c-5e711e6ff5f9.png) ![screenshot-file-transfer](https://user-images.githubusercontent.com/19966907/82762322-e1149f80-9e32-11ea-80dc-762adcbcac48.png)


## Features

 * The communication goes «directly over i2p» from client to client, no server is required.
 
## How to run it

You need to enable SAM in your router on <a href="http://127.0.0.1:7657/configclients">java i2p configclients page</a> or i2pd's i2pd.conf [sam] section to make I2P Chat work over your I2P router.

## Project status, news and history

### Project status

`2024 Dec`: Acetone made changes to make the project build on Qt6 (did not make changes for platform-dependent code not related to Linux).

### Current news

5 Jan, 2017: Original repo at http://git.repo.i2p/w/I2P-Messenger-QT.git was fully merged here.
  
### History

Original developer of this messenger went away.

## License

The license of this software is GPLv2.

### Build instructions

**Qt6**
```
sudo apt-get install git qt6-base-dev qt6-multimedia-dev build-essential
git clone https://github.com/freeacetone/i2pchat
cd i2pchat
qmake6 && make
```

## Running

On Linux, `make` creates `I2P-Messenger` executable in the current folder. Run it with `./I2P-Messenger`. When ran, switch yourself to online. It will generate your Destination address (a key) on first connect to SAM.

