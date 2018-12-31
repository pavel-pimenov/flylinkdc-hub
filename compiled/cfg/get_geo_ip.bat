del IpToCountry.csv
del IpToCountry.6R.csv
del iptocountry.zip.tmp
wget -N --tries=999 http://inethub.olvi.net.ua/ftp/db/maxmind.com/iptocountry.zip.tmp
ren iptocountry.zip.tmp iptocountry.zip
7z.exe x iptocountry*.zip -y
del iptocountry.zip
rem wget -N --tries=999 http://software77.net/geo-ip/?DL=1 --output-document=IpToCountry.csv.gz
wget -N --tries=999 http://software77.net/geo-ip/?DL=7 --output-document=IpToCountry.6R.csv.gz
7z.exe x IpToCountry*.csv.gz -y
del IpToCountry.6R.csv.gz
rem del IpToCountry.csv.gz
