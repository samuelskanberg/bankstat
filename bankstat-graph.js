(function($) {
    $(document).ready(function(){
        var graph_data = Drupal.settings.bankstat.graph_data;

        console.log(graph_data);

        google.load("visualization", "1", {packages:["corechart"]});
        google.setOnLoadCallback(drawChart);
        function drawChart() {
            var data = google.visualization.arrayToDataTable(graph_data);

            var options = {
                title: 'Utgifter över tid',
                vAxis: {title: 'Totalt'},
                isStacked: true
            };

            var chart = new google.visualization.SteppedAreaChart(document.getElementById('stepchart'));

            chart.draw(data, options);
        }

        var graph_data_positive = Drupal.settings.bankstat.graph_data_positive;

        console.log(graph_data_positive);

        google.load("visualization", "1", {packages:["corechart"]});
        google.setOnLoadCallback(drawChartPositive);
        function drawChartPositive() {
            var data = google.visualization.arrayToDataTable(graph_data_positive);

            var options = {
                title: 'Inkomst över tid',
                vAxis: {title: 'Totalt'},
                isStacked: true
            };

            var chart = new google.visualization.SteppedAreaChart(document.getElementById('stepchart_positive'));

            chart.draw(data, options);
        }
    });
})(jQuery);
