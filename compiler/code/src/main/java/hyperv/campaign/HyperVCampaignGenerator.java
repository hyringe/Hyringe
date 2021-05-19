package hyperv.campaign;

import hycallparser.parameter.Parameter;
import hycallparser.parameter.ParameterKeyValue;
import hycallparser.visitors_and_listeners.campaignListeners.CampaignListener;
import hyperv.general.ByteTransformUtils;
import hyperv.general.HyperVHypercallDict;
import org.apache.commons.io.FileUtils;
import org.apache.commons.lang3.ArrayUtils;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.*;
import java.util.stream.Collectors;

public class HyperVCampaignGenerator implements CampaignListener {

    public static final Byte TYPE_WAIT = 0x51;
    public static final Byte TYPE_CALL = (byte)0xca;

    private String filepath;
    private List<Byte> bytes;
    private Optional<HyperVHypercall> pendingCall;

    int callCount;
    int delayCount;

    public HyperVCampaignGenerator() {
        bytes = new ArrayList<>();
        pendingCall = Optional.empty();

        callCount = 0;
        delayCount = 0;
    }

    @Override
    public void setParameters(String[] args) {
        if (args.length != 1)
            throw new IllegalArgumentException("Expected only one argument (filepath where to save compiled campaign.");
        filepath = args[0];
    }

    @Override
    public void encodeHypercall(List<ParameterKeyValue> parameters) {
        callCount++;
        Map<String, Parameter> args = parameters.stream().collect(Collectors.toMap(ParameterKeyValue::getKey, ParameterKeyValue::getValue));
        if (!args.containsKey("name"))
            throw new IllegalArgumentException("\"name\" is required to identify hypercall");
        Parameter pname = args.get("name");
        if (!pname.isString())
            throw new IllegalArgumentException("\"name\" has to be a string");
        String hypercallName = pname.getString();
        int callcode = HyperVHypercallDict.getCallCode(hypercallName);
        args.remove("name");

        InputPageGenerator ipg = new InputPageGenerator(HyperVHypercallDict.getRequiredInputPageSize(hypercallName));
        List<String> paramDone = new ArrayList<>(args.size());
        for (Map.Entry<String, Parameter> e : args.entrySet()) {
            String paramName = e.getKey();
            int offset = HyperVHypercallDict.getParameterOffset(hypercallName, paramName);
            int size = HyperVHypercallDict.getParameterSize(hypercallName, paramName);

            Parameter pvalue = e.getValue();
            if (!pvalue.isInteger())
                throw new IllegalArgumentException("Value for parameter \"" + paramName + "\" has to be an integer");
            long lvalue = pvalue.getInt().longValue();
            ipg.putValue(lvalue, offset, size);

            paramDone.add(paramName);
        }
//        if (paramDone.size() < HyperVHypercallDict.getParameterCount(hypercallName)) {
//            System.out.print("Warning: Not all paramaters have been supplied have been supplied to call \"" + hypercallName + "\", filling these with zeros: ");
//            for (String paramName : HyperVHypercallDict.getAllParameters(hypercallName).stream().filter(p -> !paramDone.contains(p)).collect(Collectors.toList())) {
//                System.out.print(paramName + "  ");
//                int offset = HyperVHypercallDict.getParameterOffset(hypercallName, paramName);
//                int size = HyperVHypercallDict.getParameterSize(hypercallName, paramName);
//                ipg.putValue(0, offset, size);
//            }
//            System.out.println();
//        }

        HyperVHypercall hc = new HyperVHypercall(callcode, 1, ipg);
        if (pendingCall.isPresent()) {
            boolean b = pendingCall.get().equals(hc);
            b = b;
        }
        if (pendingCall.map(hc::equals).orElse(false)) {
            HyperVHypercall hcp = pendingCall.get();
            hcp.incCount();
            if (hcp.isMaxCount()) {
                bytes.addAll(hcp.toBytes());
                pendingCall = Optional.empty();
            }
        }
        else {
            pendingCall.ifPresent(hcp -> bytes.addAll(hcp.toBytes()));
            pendingCall = Optional.of(hc);
        }
    }

    @Override
    public void encodeDelay(long delayMicroseconds) {
        delayCount++;

        pendingCall.ifPresent(hcp -> bytes.addAll(hcp.toBytes()));
        pendingCall = Optional.empty();

        bytes.add(TYPE_WAIT);
        byte[] delay = ArrayUtils.subarray(ByteBuffer.allocate(Long.SIZE / Byte.SIZE).order(ByteOrder.LITTLE_ENDIAN).putLong(delayMicroseconds).array(), 0, 4);
        bytes.addAll(Arrays.stream(ArrayUtils.toObject(delay)).collect(Collectors.toList()));
        bytes.add((byte)0);
        bytes.add((byte)0);
    }

    @Override
    public void finish() {
        pendingCall.ifPresent(hcp -> bytes.addAll(hcp.toBytes()));

        try {
            File file = new File(filepath);
            byte[] campaign = ArrayUtils.toPrimitive(bytes.toArray(Byte[]::new));
            byte[] campaignSize = ByteTransformUtils.longToBytes(campaign.length, 4);

            FileUtils.writeByteArrayToFile(file, campaignSize);
            FileUtils.writeByteArrayToFile(file, ByteTransformUtils.longToBytes(callCount, 4), true);
            FileUtils.writeByteArrayToFile(file, ByteTransformUtils.longToBytes(delayCount, 4), true);
            FileUtils.writeByteArrayToFile(file, campaign, true);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
