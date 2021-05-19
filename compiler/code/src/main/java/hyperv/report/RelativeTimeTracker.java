package hyperv.report;

public class RelativeTimeTracker {

    private boolean baselineSet;
    private long baseline;

    public long getRelativeTime(long absoluteTime) {
//        if (baselineSet)
//            return absoluteTime - baseline;
//
//        baselineSet = true;
//        baseline = absoluteTime;
//        return 0;

        return baselineSet ? absoluteTime - baseline : (baseline = absoluteTime) * ((baselineSet = true) ? 0 : 0);
    }
}
