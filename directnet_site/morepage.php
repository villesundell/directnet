<?PHP
function morepage($page, $frame) {
    if ($page == "main") {
        // Also do news
        $handle = @fopen("http://sourceforge.net/news/?group_id=123564", "r");
        if ($handle === false) return;
        
        $mode = 0;
        while (!feof($handle)) {
            $line = trim(fgets($handle, 2048));
            
            switch ($mode) {
                case 0:
                    if ($line == "<TR><TD VALIGN=\"TOP\">") {
                        $mode++;
                    }
                    break;
                case 1:
                    if ($line == "</TD></TR></TABLE>") {
                        return;
                    }
                    
                    $temp = str_replace("HREF=\"/forum", "HREF=\"http://www.sourceforge.net/forum", $line);
                    print $temp . "\n";
                    break;
            }
        }
    }
    
    if ($page == "download-s" /*|| $page == "download-b"*/) {
        // Include linked-in downloads
        if ($page == "download-s") {
            $handle = @fopen("http://sourceforge.net/project/showfiles.php?group_id=123564&package_id=135053", "r");
        } else {
            $handle = @fopen("http://sourceforge.net/project/showfiles.php?group_id=123564&package_id=135808", "r");
        }
        $mode = 0;
        while (!feof($handle)) {
            $line = trim(fgets($handle, 2048));
            
            switch ($mode) {
                case 0:
                    if ($line == "<table width=\"100%\" border=\"0\" cellspacing=\"1\" cellpadding=\"1\">") {
                        print "$line\n";
                        $mode++;
                    }
                    break;
                case 1:
                    if ($line == "</TABLE>") {
                        print "$line\n";
                        return;
                    }
                    
                    $temp = str_replace("href=\"show", "href=\"http://www.sourceforge.net/project/show", $line);
                    $temp = str_replace("bgcolor=\"#FFFFFF\"", "bgcolor=\"#111111\"", $temp);
                    print $temp . "\n";
                    break;
            }
        }
    }
    
    if ($page == "download-b") {
        $ver = "ALPHA0.3";
        
        print "<h1>Binaries</h1>";
        print "<ul>";
        
        // GNU/Linux
        print "<li>GNU/Linux<br>";
        print "<ul>";
        print "<li><a href='http://prdownloads.sourceforge.net/directnet/directnet-" . $ver . "-i686-pc-linux-gnu.gz'>i686 (Intel/AMD PC)</a></li>";
        print "<li><a href='http://prdownloads.sourceforge.net/directnet/directnet-" . $ver . "-alphaev67-unknown-linux.gz'>Alpha</a></li>";
        print "<li><a href='http://prdownloads.sourceforge.net/directnet/directnet-" . $ver . "-x86_64-unknown-linux-gnu.gz'>x86_64 (AMD AMD64/Intel EM64T PC)</a></li>";
        print "</ul></li>";
        
        // OSX
        print "<li>Mac OS X (10.2 or higher)<br>";
        print "<ul>";
        print "<li><a href='http://prdownloads.sourceforge.net/directnet/directnet-" . $ver . "-powerpc-apple-darwin6.8.gz'>PPC</a></li>";
        print "</ul></li>";
        
        // Solaris
        print "<li>Solaris 9<br>";
        print "<ul>";
        print "<li><a href='http://prdownloads.sourceforge.net/directnet/directnet-" . $ver . "-sparc-sun-solaris2.9.gz'>ULTRASPARC</a></li>";
        print "</ul></li>";
        
        // FreeBSD
        print "<li>FreeBSD 4.10<br>";
        print "<ul>";
        print "<li><a href='http://prdownloads.sourceforge.net/directnet/directnet-" . $ver . "-i386-unknown-freebsd4.10.gz'>i686 (Intel/AMD PC)</a></li>";
        print "</ul></li>";
        
        // HP-UX
        print "<li>HP-UX 11<br>";
        print "<ul>";
        print "<li><a href='http://prdownloads.sourceforge.net/directnet/directnet-" . $ver . "-hppa2.0w-hp-hpux11.23.gz'>HPPA</a></li>";
        print "<li><a href='http://prdownloads.sourceforge.net/directnet/directnet-" . $ver . "-ia64-hp-hpux11.23.gz'>Itanium</a></li>";
        print "</ul></li>";
        
        // HP Tru64
        print "<li>HP Tru64 UNIX<br>";
        print "<ul>";
        print "<li><a href='http://prdownloads.sourceforge.net/directnet/directnet-" . $ver . "-alphaev7-dec-osf5.1b.gz'>Alpha</a></li>";
        print "</ul></li>";
        
        // Huh?
        print "<li>That ... uhhh ... I guess they call it an \"operating system\" ... it's named ... uhhh ... doors ... or glass ... or something.<br>";
        print "<ul>";
        print "<li><a href='http://prdownloads.sourceforge.net/directnet/directnet-" . $ver . "-i686-pc-cygwin.zip'>i686 (Intel/AMD PC (<b>NOT</b> Wintel you ignorant morons)</a></li>";
        print "</ul></li></ul>";
    }
}
?>