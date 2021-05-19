package hyperv.campaign;

import hyperv.general.ByteTransformUtils;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

public class HyperVHypercall {

    private int callCode;
    private int count;
    private InputPageGenerator ipg;

    public HyperVHypercall(int callCode, int count, InputPageGenerator ipg) {
        this.callCode = callCode;
        this.count = count;
        this.ipg = ipg;
    }

    public List<Byte> toBytes() {
        List<Byte> result = new ArrayList<>(1+2+2+ipg.getSize());
        result.add(HyperVCampaignGenerator.TYPE_CALL);
        result.addAll(ByteTransformUtils.longToByteList(callCode, 2));
        result.addAll(ByteTransformUtils.longToByteList(count, 2));
        result.addAll(ipg.getPage());
        return result;
    }

    public void incCount() {
        count++;
    }

    public boolean isMaxCount() {
        return count == 0xffff;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        HyperVHypercall that = (HyperVHypercall) o;
        return callCode == that.callCode &&
                ipg.equals(that.ipg);
    }

    @Override
    public int hashCode() {
        return Objects.hash(callCode, ipg);
    }
}
