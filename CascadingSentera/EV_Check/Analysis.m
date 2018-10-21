% Analysis
% loads jpg, plots histogram of pixel values

% ----- INPUT -----
% imgName: enter name of image of interest. It will be the same in both the
% nRGB and NDRE folders.
imgName = 'outdoor_086.jpg';
% ----- End Input: just hit run -----

rgbImgPath = ['RGB/' imgName];
nirImgPath = ['NIR/' imgName];

rgbRaw = imread(rgbImgPath);
nirRaw = imread(nirImgPath);

rgbRaw = imresize(rgbRaw, 1);
nirRaw = imresize(nirRaw, 1);

% show images
figure(1); imshow(rgbRaw);
figure(2); imshow(nirRaw);

% show histograms
figure(3);
subplot(3,1,1); histogram(rgbRaw(:,:,1)); title("RGB: Red Band"); xlabel("Pixel Value"); ylabel("Number of Pixels");
subplot(3,1,2); histogram(rgbRaw(:,:,2)); title("RGB: Green Band"); xlabel("Pixel Value"); ylabel("Number of Pixels");
subplot(3,1,3); histogram(rgbRaw(:,:,3)); title("RGB: Blue Band"); xlabel("Pixel Value"); ylabel("Number of Pixels");
figure(4);
subplot(3,1,1); histogram(nirRaw(:,:,1)); title("NIR: Red Band"); xlabel("Pixel Value"); ylabel("Number of Pixels");
subplot(3,1,2); histogram(nirRaw(:,:,2)); title("NIR: Green Band"); xlabel("Pixel Value"); ylabel("Number of Pixels");
subplot(3,1,3); histogram(nirRaw(:,:,3)); title("NIR: Blue Band"); xlabel("Pixel Value"); ylabel("Number of Pixels");
