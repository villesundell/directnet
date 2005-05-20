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
        $handle = @fopen("http://sourceforge.net/project/showfiles.php?group_id=123564&package_id=135053", "r");
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
        $ver = "alpha0.4";
        
        print "<h1>Binaries</h1>";
        print "<ul>";
        
        // GNU/Linux
        print "<li>GNU/Linux<br>";
        print "<ul>";
        print "<li><a href='http://prdownloads.sourceforge.net/directnet/directnet-" . $ver . "-i386-linux-gnu.tar.gz?download'>i686 (Intel/AMD PC)</a></li>";
        print "<li><a href='http://prdownloads.sourceforge.net/directnet/directnet-" . "ALPHA0.3" . "-alphaev67-unknown-linux.gz?download'>Alpha (old)</a></li>";
        print "<li><a href='http://prdownloads.sourceforge.net/directnet/directnet-" . "ALPHA0.3" . "-x86_64-unknown-linux-gnu.gz?download'>x86_64 (AMD AMD64/Intel EM64T PC) (old)</a></li>";
        print "</ul></li>";
        
        // OSX
        print "<li>Apple Mac OS X<br>";
        print "<ul>";
        print "<li><a href='http://prdownloads.sourceforge.net/directnet/directnet-" . $ver . "-macosx.dmg.gz?download'>PPC</a></li>";
        print "</ul></li>";
        
        // Solaris
        print "<li>Sun Solaris 9<br>";
        print "<ul>";
        print "<li><a href='http://prdownloads.sourceforge.net/directnet/directnet-" . "ALPHA0.3" . "-sparc-sun-solaris2.9.gz?download'>ULTRASPARC (old)</a></li>";
        print "</ul></li>";
        
        // FreeBSD
        print "<li>FreeBSD 4.10<br>";
        print "<ul>";
        print "<li><a href='http://prdownloads.sourceforge.net/directnet/directnet-" . "ALPHA0.3" . "-i386-unknown-freebsd4.10.gz?download'>i686 (Intel/AMD PC) (old)</a></li>";
        print "</ul></li>";
        
        // HP-UX
        print "<li>HP-UX 11<br>";
        print "<ul>";
        print "<li><a href='http://prdownloads.sourceforge.net/directnet/directnet-" . "ALPHA0.3" . "-hppa2.0w-hp-hpux11.23.gz?download'>HPPA (old)</a></li>";
        print "<li><a href='http://prdownloads.sourceforge.net/directnet/directnet-" . "ALPHA0.3" . "-ia64-hp-hpux11.23.gz?download'>Itanium (old)</a></li>";
        print "</ul></li>";
        
        // HP Tru64
        print "<li>HP Tru64 UNIX<br>";
        print "<ul>";
        print "<li><a href='http://prdownloads.sourceforge.net/directnet/directnet-" . "ALPHA0.3" . "-alphaev7-dec-osf5.1b.gz?download'>Alpha (old)</a></li>";
        print "</ul></li>";
        
        // Windows
        print "<li>Microsoft Windows<br>";
        print "<ul>";
        print "<li><a href='http://prdownloads.sourceforge.net/directnet/directnet-" . $ver . "-win32.zip?download'>i686 (Intel/AMD PC)</a></li>";
        print "</ul></li></ul>";
    }
}
?>