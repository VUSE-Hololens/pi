% MergeToNDVI
% Script for merging RGB and NDRE images taken by the Sentera into a single
% NDVI image
% Instructions: Fill out the two fields in the Input section below, then
% run. The NDVI image will appear in a popup from which it can be saved as a
% .jpg


% ----- INPUT -----
% session: enter name of session. This will also be the directory name.
session = 'Grass_2';
% imgName: enter name of image of interest. It will be the same in both the
% nRGB and NDRE folders.
imgName = 'IMG_00010.jpg';

% ----- End Input: just hit run -----

%rgbImgPath = [session '/RGB/' imgName];
%nirImgPath = [session '/NIR/' imgName];
rgbImgPath = 'bush2_rgb.jpg';
nirImgPath = 'bush2_nir.jpg';

rgbRaw = imread(rgbImgPath);
nirRaw = imread(nirImgPath);

rgbRaw = imresize(rgbRaw, 1);
nirRaw = imresize(nirRaw, 1);

rgbInfo = imfinfo(rgbImgPath);
nirInfo = imfinfo(nirImgPath);

rgbCamInfo = rgbInfo.DigitalCamera;
nirCamInfo = nirInfo.DigitalCamera;

% exposure: the image exposure time (in secons) for the RGB and NIR images
ev_rgb = rgbCamInfo.ExposureTime;
ev_nir = nirCamInfo.ExposureTime;
% ISO: the image ISO for the RGB and NIR images
iso_rgb = rgbCamInfo.ISOSpeedRatings;
iso_nir = nirCamInfo.ISOSpeedRatings;

bandDataSep = zeros(size(rgbRaw,1), size(rgbRaw,2), 5);
bandDataNorm = zeros(size(rgbRaw,1), size(rgbRaw,2), 5);

% remove cross-band interferance
bandDataSep(:,:,1) = -0.061*rgbRaw(:,:,1) - 0.182*rgbRaw(:,:,2) + 1.377*rgbRaw(:,:,3); % blue
bandDataSep(:,:,2) = -0.329*rgbRaw(:,:,1) + 1.420*rgbRaw(:,:,2) - 0.199*rgbRaw(:,:,3); % green
bandDataSep(:,:,3) = +1.150*rgbRaw(:,:,1) - 0.110*rgbRaw(:,:,2) - 0.034*rgbRaw(:,:,3); % red
bandDataSep(:,:,4) = +1.000*nirRaw(:,:,1) - 0.956*nirRaw(:,:,3); % red edge
bandDataSep(:,:,5) = -0.341*nirRaw(:,:,1) + 2.436*nirRaw(:,:,3); % NIR

% account for camera's sensitivity difference (Step 1)
bandDataNorm(:,:,1:3) = bandDataSep(:,:,1:3) / (ev_rgb * (iso_rgb / 100));
bandDataNorm(:,:,4:5) = bandDataSep(:,:,4:5) / (ev_nir * (iso_nir / 100));

% account for camera's sensitivity difference (Step 2)
% k = 255 / max(bandDataNorm(:));
% bandDataNorm = bandDataNorm .* k;
% rgb_coef = k / (ev_rgb * (iso_rgb / 100));
% nir_coef = k / (ev_nir * (iso_nir / 100));



% calculate NDVI
ndvi = (2.7 .* bandDataNorm(:,:,5) - bandDataNorm(:,:,3)) ./ (2.7 .* bandDataNorm(:,:,5) + bandDataNorm(:,:,3));

% clamp 0-1
%ndvi(ndvi<0) = 0;

% grab images
rgbSep = zeros(size(rgbRaw,1), size(rgbRaw,2), 3);
rgbSep(:,:,1) = bandDataSep(:,:,3);
rgbSep(:,:,2) = bandDataSep(:,:,2);
rgbSep(:,:,3) = bandDataSep(:,:,1);

nirSep = zeros(size(rgbRaw,1), size(rgbRaw,2), 3);
nirSep(:,:,1) = bandDataSep(:,:,5); % red
nirSep(:,:,2) = bandDataSep(:,:,5); % green
nirSep(:,:,3) = bandDataSep(:,:,4); % blue

rgbNorm = zeros(size(rgbRaw,1), size(rgbRaw,2), 3);
rgbNorm(:,:,1) = bandDataNorm(:,:,3);
rgbNorm(:,:,2) = bandDataNorm(:,:,2);
rgbNorm(:,:,3) = bandDataNorm(:,:,1);

nirNorm = zeros(size(rgbRaw,1), size(rgbRaw,2), 3);
nirNorm(:,:,1) = bandDataNorm(:,:,5);
nirNorm(:,:,2) = bandDataNorm(:,:,5);
nirNorm(:,:,3) = bandDataNorm(:,:,4);


figure(1); 
subplot(2,3,1); imshow(uint8(rgbSep)); title("RGB: Band Separated");
subplot(2,3,2); imshow(uint8(rgbSep(:,:,1))); title("RGB: Band Separated, Red Only");
subplot(2,3,3); histogram(rgbSep(:,:,1)); title("RGB: Band Separated, Red Only");
subplot(2,3,4); imshow(uint8(nirSep)); title("NIR: Band Separated");
subplot(2,3,5); imshow(uint8(nirSep(:,:,1))); title("NIR: Band Separated, NIR Only");
subplot(2,3,6); histogram(nirSep(:,:,1)); title("NIR: Band Separated, NIR Only");
figure(2); 
subplot(2,3,1); imshow(uint8(rgbNorm)); title("RGB: Normalized");
subplot(2,3,2); imshow(uint8(rgbNorm(:,:,1))); title("RGB: Normalized, Red Only");
subplot(2,3,3); histogram(rgbNorm(:,:,1)); title("RGB: Normalized, Red Only");
subplot(2,3,4); imshow(uint8(nirNorm)); title("NIR: Normalized");
subplot(2,3,5); imshow(uint8(nirNorm(:,:,1))); title("NIR: Normalized, NIR Only");
subplot(2,3,6); histogram(nirNorm(:,:,1)); title("NIR: Normalized, NIR Only");
figure(3);
subplot(1,2,1); imshow(ndvi); title("NDVI");
subplot(1,2,2); histogram(ndvi); title("NDVI");
% subplot(2,2,3); imshow(ndre); title("NDRE");
% subplot(2,2,4); histogram(ndre); title("NDRE");
