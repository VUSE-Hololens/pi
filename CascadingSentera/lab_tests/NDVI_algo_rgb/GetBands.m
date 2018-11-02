% GetBands
% pulls R, G, B bands from rgb image

filepath = 'img.jpg';

img = imread(filepath);
band1 = img(:,:,1);
band2 = img(:,:,2);
band3 = img(:,:,3);

figure(1);
subplot(2,2,1); imshow(img); title('All Bands');
subplot(2,2,2); imshow(band1); title('Band1');
subplot(2,2,3); imshow(band2); title('Band2');
subplot(2,2,4); imshow(band3); title('Band3');
