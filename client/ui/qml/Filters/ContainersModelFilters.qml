pragma Singleton

import QtQuick 2.15

import SortFilterProxyModel 0.2

Item {
    ValueFilter {
        id: vpnTypeFilter
        roleName: "isVpnContainer"
        value: true
    }

    ValueFilter {
        id: installedFilter
        roleName: "isInstalled"
        value: true
    }

    // Fork: no install/services UI, only installed VPN protocols are ever listed
    function getReadAccessProtocolsListFilters() {
        return [vpnTypeFilter, installedFilter]
    }
}
