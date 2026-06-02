#!/usr/bin/awk -f

# This script converts the first device tree in the second one. It essentially
# peels the first two levels of the device tree and embeds the remaining content
# in a device tree overlay.

# / {
#         amba_pl: amba_pl {
#                 (...)
#         };
# };

# /dts-v1/;
# /plugin/;
#
# / {
#         fragment@0 {
#                 target-path = "/fpga-full";
#                 __overlay__ {
#                         (...)
#                 };
#         };
# };


# https://docs.kernel.org/driver-api/fpga/fpga-region.html
# https://www.kernel.org/doc/Documentation/devicetree/bindings/fpga/fpga-region.txt

function push_if_eq(n) {
    if (level == n) { level++ } else { print "bad" >> "/dev/stderr"; exit 1 }
}

function pop_if_lt(n) {
    if (level < n) { level-- } else { print "bad" >> "/dev/stderr"; exit 1 }
}

function check_eq(n) {
    if (level == n) { ; } else { print "bad" >> "/dev/stderr"; exit 1 }
}

BEGIN {
    print "/dts-v1/;\n/plugin/;\n"
    print "/ {"
    print "\tfragment@0 {"
    print "\t\ttarget-path = \"/fpga-full\";"
    print "\t\t__overlay__ {"

    level = 0
}

/^\/ \{/                 { push_if_eq(0); next }
/^\tamba_pl: amba_pl \{/ { push_if_eq(1); next }
/^\t{0,1}\};$/           { pop_if_lt(3);  next }
/^\t*$/                  { next }
                         { check_eq(2);
    # This should only work after having matched /^\t\tafi0.*{$/
    if (match($0, /^\t\t\tcompatible = "xlnx,afi-fpga";$/)) {
        print "\t\t\t\tcompatible = \"xlnx,zynq-afi-fpga\";"
    } else {
        print "\t" $0
    }
}

END {
    check_eq(0);
    print "\t\t}; /* __overlay__ */"
    print "\t}; /* fragment@0 */"
    print "\tfragment@1 {"
    print "\t\ttarget-path = \"/\";"
    print "\t\t__overlay__ {"
    print "\t\t\treserved-memory {"
	print "\t\t\t\t#address-cells = <1>;"
	print "\t\t\t\t#size-cells = <1>;"
    print "\t\t\t\tranges;"
    print "\t\t\t\tlinux_cma: linux,cma@0x1C000000 {"
	print "\t\t\t\t\tcompatible = \"shared-dma-pool\";"
	print "\t\t\t\t\treusable;"
	print "\t\t\t\t\treg = <0x1C000000 0x04000000>; /* Reserves 64MB out of 512MB RAM starting from address 0x1C000000*/"
	print "\t\t\t\t\tlinux,cma-default;"
    print "\t\t\t\t};"
	print "\t\t\t};"
    print "\t\t}; /* __overlay__ */"
    print "\t}; /* fragment@1 */"
    print "}; /* / */"
}
