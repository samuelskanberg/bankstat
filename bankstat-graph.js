(function($) {
    $(document).ready(function(){
        console.log("Yihaa");
        var graph_data = Drupal.settings.bankstat.graph_data;

        console.log(graph_data);

        google.load("visualization", "1", {packages:["corechart"]});
        google.setOnLoadCallback(drawChart);
        function drawChart() {

            //var data = google.visualization.arrayToDataTable([
            //    ['Director (Year)',  'Rotten Tomatoes', 'IMDB'],
            //    ['Alfred Hitchcock (1935)', 8.4,         7.9],
            //    ['Ralph Thomas (1959)',     6.9,         6.5],
            //    ['Don Sharp (1978)',        6.5,         6.4],
            //    ['James Hawes (2008)',      4.4,         6.2]
            //    ]);
            var data = google.visualization.arrayToDataTable(graph_data);

            var options = {
                title: 'Utgifter över tid',
                vAxis: {title: 'Totalt'},
                isStacked: true
            };

            var chart = new google.visualization.SteppedAreaChart(document.getElementById('stepchart'));

            chart.draw(data, options);
        }
    });
})(jQuery);
