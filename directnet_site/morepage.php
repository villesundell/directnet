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
    
    if ($page == "download") {
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
}
?>