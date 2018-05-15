setwd("C:/Users/sdgeo/Dropbox/Own/Post-Grad-UNC/Comp530/comp530H-f17/comp530H-f17/HW5/")
dat <- read.csv(file = "ParsedRData.csv", header = T)
weights <- as.vector(dat$height_white)
times <- as.vector(dat$times)
widths <- times/1000
height_white <- weights
height_black <- rep(2, times = 72)

df <- data.frame(height_white,height_black)
mat <- t(as.matrix(df))

jpeg("process_time_output.jpg", height = 15, width = 20, units = "in", res = 300)
barplot(mat, col = c("white","black"), space = 0, width = widths, 
        border = "white",
        xlab = "Time", ylab = "Process Weight",
        ylim = c(0,20))
axis(1, pos = 0, labels = NA, at = c(0))
dev.off()


