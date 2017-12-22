#!/usr/bin/python3

import subprocess

devices = subprocess.getoutput("lspci -n | grep 14e4:")
ids = ['4311', '4312', '4313', '4315', '4727', '4328', '4329', '432a', '432b', '432c', '432d', '4365', '4353', '4357', '4358', '4359', '4331', '43a0']

for id in ids:
    pci_id = "14e4:%s" % id

    if pci_id in devices:
        print("broadcom-sta-dkms")
