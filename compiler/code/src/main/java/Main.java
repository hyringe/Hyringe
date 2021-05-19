import hycallparser.antlr.HycallLexer;
import hycallparser.antlr.HycallParser;
import hycallparser.preprocessor.CampaignFileLoader;
import hycallparser.visitors_and_listeners.*;
import hycallparser.visitors_and_listeners.campaignListeners.CampaignListener;
import hycallparser.visitors_and_listeners.campaignListeners.CampaignListenerManager;
import org.antlr.v4.runtime.CharStream;
import org.antlr.v4.runtime.CharStreams;
import org.antlr.v4.runtime.CommonTokenStream;
import org.apache.commons.lang3.ArrayUtils;

import java.io.IOException;
import java.util.Arrays;

public class Main {

    private Main(){}

    public static void main(String[] args) throws IOException {
        if (args.length == 0) {
            throw new IllegalArgumentException("Expected at least one argument: the path to the .hycall file");
        }

        String fileContent = CampaignFileLoader.fromFile(args[0]);
        CharStream input = CharStreams.fromString(fileContent);
        HycallLexer lexer = new HycallLexer(input);
        CommonTokenStream tokens = new CommonTokenStream(lexer);
        HycallParser parser = new HycallParser(tokens);

        ProgramStructureListener psl = new ProgramStructureListener();
        parser.program().enterRule(psl);
        ProgramState program = psl.getProgramState();
        CampaignListenerManager.loadCampaignListener(ArrayUtils.subarray(args, 1, args.length));
        long start = System.currentTimeMillis();
        new ProgramExecutor(program).execute();

        CampaignListenerManager.getActiveCampaignListener().finish();

        long end = System.currentTimeMillis();
        System.out.println("Compile time: " + (end -  start));
    }
}
