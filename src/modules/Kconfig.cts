menu "CTS"

config LIBREWATCH_CTS
    bool "Enable Current Time Service module"
    depends on CAF
    depends on DATE_TIME
    depends on !DATA_TIME_NTP
    help
      This module subscribe to central's CTS service and uses datetime to update systems clock.

if LIBREWATCH_CTS

config LIBREWATCH_CTS_BACKUP
    bool "Enable it to retain datetime value during software reset"
    default y

config LIBREWATCH_CTS_BACKUP_INTERVAL_SECONDS
    int "CTS backup interval"
    default 5
    depends on LIBREWATCH_CTS_BACKUP
    help
      How often datetime value gets backed up

module = LIBREWATCH_CTS
module-str = cts
source "subsys/logging/Kconfig.template.log_config"

endif

endmenu
