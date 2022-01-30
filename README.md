# Schlock

>  Something, such as merchandise or literature, that is inferior or poorly made.

Schlock is a fork of [Swaylock](README-SWAYLOCK.md) for use on
touchscreens: instead of using keyboard entry and the user's
regular password, it provides an onscreen numeric pad on which
you can type your PIN. You choose a PIN while configuring the
program: it is not connected with your regular Unix password.

## Status

![Abandon all hope](https://img.shields.io/badge/abandon-all%20hope-red)

> "[It] is both good and original; but the part that is good is not
original, and the part that is original is not good." - Samuel
Johnson, attrib.

This is a pre-release which is published on Github mostly so I have a
backup of the code. Don't trust it. It has not been
security-audited. There will be bugs.  There is dead code. You should
read the Security section.

## Installation

The installation process is unchanged from
[Swaylock](README-SWAYLOCK.md), save for the
additional library dependency on
[Libsodium](https://libsodium.gitbook.io/doc/installation)

## Setup

Generate a PIN file by running

    mkpin > $HOME/.config/schlock.pin
	chmod 0400  $HOME/.config/schlock.pin

(This is not a suggested pin file location, just an example)

Start schlock with `PIN_FILE=$HOME/.config/schlock.pin schlock`

## Security

* The threat model is "my five year old child picks up my phone and
  starts pressing things at random". If your attacker is older than
  five (or is especially precocious) this app may not address your
  needs.

* [The security of customer-chosen banking
  PINs](https://www.cl.cam.ac.uk/~rja14/Papers/BPA12-FC-banking_pin_security.pdf)
  by Joseph Bonneau, S̈oren Preibusch, and Ross Anderson finds that we
  should expect a competent attacker to guess one in around every 15
  customer-chosen PINS. My takeaway from reading that is that you
  should generate a PIN randomly instead of choosing a "memorable"
  one, and you should reject the PIN and generate another if it
  is listed in their suggested blocklist (Appx B).

* Even an arbitrary four or six digit numeric PIN is always going to
  be more guessable than a long alphanumeric password. Schlock tries
  to mitigate this for online attacks by enforcing a timeout between
  failed attempts, and for offline attacks by hashing the secret using
  Argon2 (via libsodium).

* There has been significant effort by other people working on
  Swaylock and the Wayland protocols/lbraries to fix bugs like
  "plugging and unplugging the keyboard may crash the locker",
  most of which work this app benefits from. I may have introduced
  more bugs in the new bit, of course.

## Next steps

- remove some debug logging

- scale to screen size (or to thumb reach? may need this to be
configurable)

- visual feedback when buttons pressed

- more aesthetic "wrong password" signalling, e.g. add some padding
  around the red background area

- maybe find a better place for backspace and enter

- use command line parameter for pin file location instead
  of environment variable

- try on an actual touch device instead of assuming pointer emulation
  will work

- refine the backoff timings. maybe you get three attempts before
  the delay cuts in but then the delay is longer.

- enhance mkpin:
  - generate random PINs
  - check PINs against blocklist
  - write the output file, so it can set the umask
