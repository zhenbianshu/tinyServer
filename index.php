<?php
$arg_str = trim($argv[1], '?/');
parse_str($arg_str, $params);
echo "请求参数列表:<table border='1px red solid'><tr><th>参数</th><th>值</th></tr>";
foreach ($params as $key => $value) {
    echo "<tr><td>".$key . "</td><td>" . $value."</td></tr>";
}
echo "</table>";