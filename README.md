# nx-hbloaderloader
A small KIP that:
 * patches FS to re-insert nspwn (a vulnerability that was patched in 5.0.0).
 * and redirects the Album title to the sdcard path `bootloader/hbl.nsp`

## Usage
With hekate:

* Build nx-hbloader and put it on your sdcard at `bootloader/hbl.nsp`
* Build nx-hbloaderloader and put the resulting kip at `bootloader/nxldrldr.kip`
* Add it to the `bootloader/hekate_ipl.ini` (it needs to have debug-mode):

```
kip1=bootloader/nxldrldr.kip
debugmode=1
```
