
import View from 'girder/views/View';
import { apiRoot } from 'girder/rest';
import { formatSize } from 'girder/misc';

import router from '../router';
import editRegionOfInterest from '../templates/dialogs/editRegionOfInterest.pug';
import '../stylesheets/panels/zoomWidget.styl';

var EditRegionOfInterest = View.extend({
    events: {
        'click .h-submit': 'downloadArea',
        'change .update-form': 'updateform'
    },

    initialize() {
        this._sizeCte = 100;
        this._format = 'JPEG';    // JPEG is the default format
        this._zoom = 0;
        this._compressionRatio = 0.2;
        // set defaults has to change
        this._width = 0;
        this._height = 0;
        this._maxMag = 20;
        this._maxZoom = 8;    // have to put the real value of all these variables
    },

    render() {        // Has to be re-structured
        var magnification = this.areaElement.zoom;
        if (magnification <= 1) {
            magnification = 1;
        } else if (magnification >= this._maxMag) {
            magnification = this._maxMag;
        }

        this.$el.html(
            editRegionOfInterest({
                magnification: magnification,
                element: this.areaElement,
                numberOfPixel: this.getNumberPixels(),
                fileSize: this.getConvertFileSize()
            })
        ).girderModal(this);
    },

    /**
     * Convert from zoom level to magnification.
     */
    zoomToMagnification(zoom) {
        return Math.round(parseFloat(this._maxMag) *
            Math.pow(2, zoom - parseFloat(this._maxZoom)) * 10) / 10;
    },

    /**
     * Convert from magnification to zoom level.
     */
    magnificationToZoom(magnification) {
        return parseFloat(this._maxZoom) -
            Math.log2(this._maxMag / magnification);
    },

    /**
     * Get the number of pixel in the region of interest
     */
    getNumberPixels() {
        var factor = Math.pow(2, this._zoom - this._maxZoom);
        var scaleWidth = factor * this._width;
        var scaleHeight = factor * this._height;
        var Npixel = scaleWidth * scaleHeight;
        return Npixel;
    },

    /**
     * Get the size of the file before download it for an image in 24b/px (result in Bytes)
     */
    getFileSize() {
        var fileSize = (this.getNumberPixels() * 3 + this._sizeCte) * this._compressionRatio;
        return fileSize;
    },

    /**
     * Get the size of the file in the appropriate unity (Bytes, MB, GB...)
     */
    getConvertFileSize() {
        var Nbytes = this.getFileSize();
        var convertedSize = formatSize(Nbytes);
        if (Nbytes >= 2 ** 30) {
            this.downloadDisable(true);
        } else {
            this.downloadDisable(false);
        }
        return convertedSize;
    },

    /**
     * Disable the Download button if SizeFile > 1GB
     */
    downloadDisable(bool) {
        var element = $('#msgDisable').attr('id');
        if (bool === true) {
            $('#download-submit').attr('disabled', 'disabled');
            if (typeof element === typeof undefined) {
                var msgDisable = $('<span></span>').text('Size > 1GB : Impossible Download ');
                msgDisable.attr('id', 'msgDisable');
                msgDisable.css({'color': 'red', 'margin-right': '120px'});
                $('#download-area-link').before(msgDisable);
            }
        } else if (bool === false) {
            $('#download-submit').removeAttr('disabled');
            if (typeof element !== typeof undefined) {
                $('#msgDisable').remove();
            }
        } else {
            console.log('Error in \'downloadDisable\' function');
        }
    },

    /**
     * Get the size of the file before download it
     */
    updateform(evt) {
        // Find the good compresion ration there are random now
        var selectedOption = $('#download-image-format option:selected').text();
        switch (selectedOption) {
            case 'JPEG':     //     JPEG
                this._format = 'JPEG';
                this._compressionRatio = 0.4;
                break;
            case 'PNG':     //  PNG
                this._format = 'PNG';
                this._compressionRatio = 0.5;
                break;
            case 'TIFF':     // TIFF
                this._format = 'TIFF';
                this._compressionRatio = 0.6;
                break;
            default:     // JPEG is the default format
                this._compressionRatio = 0.4;
        }
        this._zoom = Math.round(this.magnificationToZoom(parseFloat($('#h-element-mag').val())));
        var factor = Math.pow(2, this._zoom - this._maxZoom);
        $('#h-element-width').val(factor * this._width);
        $('#h-element-height').val(factor * this._height);
        $('#nb-pixel').val(this.getNumberPixels());
        var fileSize = this.getConvertFileSize();
        $('#size-file').val(fileSize);
    },

    /**
     * Get all data from the form and set the attributes of the
     * Region of Interest (triggering a change event).
     */
    downloadArea(evt) {
        var imageId = router.getQuery('image');
        var left = this.areaElement.left;
        var top = this.areaElement.top;
        var right = left + this._width;
        var bottom = top + this._height;
        var magnification = parseFloat($('#h-element-mag').val());
        var urlArea = apiRoot + '/item/' + imageId + '/tiles/region?' +
            $.param({
                regionWidth: this._width,
                regionHeight: this._height,
                left: left,
                top: top,
                right: right,
                bottom: bottom,
                encoding: this._format,
                contentDisposition: 'attachment',
                magnification: magnification });
        window.location.href = urlArea;
        this.$el.modal('hide');
    }
});

/**
 * Create a singleton instance of this widget that will be rendered
 * when `show` is called.
 */
var dialog = new EditRegionOfInterest({
    parentView: null
});

/**
 * Show the edit dialog box.  Watch for change events on the passed
 * `ElementModel` to respond to user submission of the form.
 *
 * @param {ElementModel} areaElement The element to edit
 * @returns {EditRegionOfInterest} The dialog's view
 */
function show(areaElement) {
    dialog.areaElement = areaElement;
    dialog._width = parseFloat(areaElement.width);
    dialog._height = parseFloat(areaElement.height);
    dialog._maxMag = parseFloat(areaElement.maxMag);
    dialog._maxZoom = parseFloat(areaElement.maxZoom);
    dialog.setElement('#g-dialog-container').render();
    return dialog;
}

export default show;
