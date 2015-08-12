#! /usr/bin/perl -w
# convert binary log file to ascii format 
# by ejyoon

use warnings;

use constant {
	PROVMSG_BOOT => 0,
 	PRVMSG_CREDFORK => 1,
	PROVMSG_CREDCOMMIT => 2,
	PROVMSG_CREDFREE => 3,
	PROVMSG_EXEC => 4,
	PROVMSG_INODE => 5,
	PROVMSG_MMAP => 6,
	PROVMSG_FILE => 7,
};

$template_hdr = "I L";
$template_fork = "L";
$template_commit = "L L L L L L L L";
$template_exec = "H[32] Q I";
$template_inode = "H[32] Q I";
$template_mmap = "H[32] Q Q";
$template_file = "H[32] Q I";

if( $#ARGV < 0 ) {
	$INP = STDIN;
	$OUTP = STDOUT;
}
else {
	open($INP, "<$ARGV[0]") or die("Cannot open file '$ARGV[0]' for reading\n");
	open($OUTP, ">$ARGV[0].out") or die("Cannot open file '$ARGV[0].out' for writing\n");
}
binmode($INP);

$headersize = length(pack($template_hdr, ( )));

while (read($INP, $header, $headersize)) {
	($hdrtype, $credid) = unpack($template_hdr, $header);

	if ($hdrtype == PROVMSG_BOOT) {
		printf $OUTP "[%x] boot\n", $credid;
	}
	elsif ($hdrtype == PRVMSG_CREDFORK) {
		$recordsize = length(pack($template_fork, ( )));
		read($INP, $record, $recordsize);
		($forkcred) = unpack($template_fork, $record);
		printf $OUTP "[%x] credfork %x\n", $credid, $forkcred;
	
	}
	elsif ($hdrtype == PROVMSG_CREDCOMMIT) {
		$recordsize = length(pack($template_commit, ( )));
		read($INP, $record, $recordsize);
		($uid, $gid, $suid, $sgid, $euid, $egid, $fsuid, $fsgid) = unpack($template_commit, $record);
		printf $OUTP "[%x] credcommit:%u:%u:%u:%u:%u:%u:%u:%u\n", $credid, $uid, $gid, $suid, $sgid, $euid, $egid, $fsuid, $fsgid;
	}
	elsif ($hdrtype == PROVMSG_CREDFREE) {
		printf $OUTP "[%x] credfree\n", $credid;
	}
	elsif ($hdrtype == PROVMSG_EXEC) {
		$recordsize = length(pack($template_exec, ( )));
		read($INP, $record, $recordsize);
		($uuid, $ino, $envplen) = unpack($template_exec, $record);
		read($INP, $record, $envplen);
		($envp) = unpack("a[$envplen]", $record);
		$envp =~ s/\0/ /g;	
		printf $OUTP "[%x] exec %s\n", $credid, $envp;
		#printf $OUTP "[%x] exec:%s:%x:%s\n", $credid, $uuid, $ino, $envplen, $envp;
	}
	elsif ($hdrtype == PROVMSG_INODE) {
		$recordsize = length(pack($template_inode, ( )));
		read($INP, $record, $recordsize);
		($uuid, $ino, $mask) = unpack($template_inode, $record);
		printf $OUTP "[%x] inode:%s:%x:%d\n", $credid, $uuid, $ino, $mask;
	}
	elsif ($hdrtype == PROVMSG_MMAP) {
		$recordsize = length(pack($template_mmap, ( )));
		read($INP, $record, $recordsize);
		($uuid, $ino, $flags) = unpack($template_mmap, $record);
		printf $OUTP "[%x] inode:%s:%x:%x\n", $credid, $uuid, $ino, $flags;
	}
	elsif ($hdrtype == PROVMSG_FILE) {
		$recordsize = length(pack($template_file, ( )));
		read($INP, $record, $recordsize);
		($uuid, $ino, $mask) = unpack($template_file, $record);
		printf $OUTP "[%x] file:%s:%x:%d\n", $credid, $uuid, $ino, $mask;
	}
	else {
		printf $OUTP "[undefined type] %x\n", $credid;

	}

}

close($INP);
close($OUTP);
	
