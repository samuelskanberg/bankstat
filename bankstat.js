(function($) {
    $(document).ready(function(){
        $('#applyscriptbutton').click(function() {
            var scriptText = $('#scripttextfield').val();

            // For each checkbox, see if the label contains the text in the textbox
            $("#selected_nids input[type='checkbox']").each(function() {
                var labelText = $("label[for='"+$(this).attr("id")+"']").text();
                if (labelText.indexOf(scriptText) >= 0) {
                        $(this).attr('checked', 'checked')
                }
            });
            return false;
        });

        $('#expense_tags').change(function() {
            console.log('tick. val: '+$(this).val());
            if ($(this).val() === "0") {
                $('.form-item-new-tag').show();
            } else {
                $('.form-item-new-tag').hide();
            }
        });
    });
})(jQuery);
