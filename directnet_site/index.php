<HTML><head><title>DirectNet</title></head>

<body bgcolor="#000000" text="#FFFFFF" link="#FFFF88" vlink="#FFFF88" alink="#AAFFAA">

<table border=0 width=100% height=100% cellspacing=0 cellpadding=5>
<tr><td align=center bgcolor="#FFFFFF">
<center><img src=dnlogo.png></center>
</td><td rowspan=2 bgcolor="#FFFFFF">
<?PHP
readfile("menu.html");
?>
</td></tr>
<tr><td width=100% height=100% bgcolor="#000000">
<?PHP
$page = $HTTP_GET_VARS['p'];
if (!$page) $page = "main";
$page = str_replace(".", "", str_replace("/", "", $page));
$page = str_replace("-g-", "guide/", $page);
@readfile("pages/$page.html");

include("morepage.php");
morepage($page, @$HTTP_GET_VARS['f']);
?>
<hr>
Hosting generously provided by <A href="http://sourceforge.net">
<IMG src="http://sourceforge.net/sflogo.php?group_id=107979&amp;type=5"
width="210" height="62" border="0" alt="SourceForge.net" /></A>
</td></tr>
</table>

</body></HTML>